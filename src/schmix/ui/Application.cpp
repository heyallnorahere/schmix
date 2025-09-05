#include "schmixpch.h"
#include "schmix/ui/Application.h"

#include "schmix/script/Bindings.h"
#include "schmix/script/Plugin.h"

namespace schmix {
    static Application* s_App;

    int Application::Run(int argc, const char** argv) {
        if (s_App != nullptr) {
            return -1;
        }

        std::vector<std::string> arguments(argc);
        for (int i = 0; i < argc; i++) {
            arguments[i] = argv[i];
        }

        Application app(arguments);
        s_App = &app;

        app.Loop();
        int status = s_App->m_Status;

        s_App = nullptr;
        return status;
    }

    Application& Application::Get() { return *s_App; }

    Application::~Application() {
        if (m_ManagedType != nullptr) {
            m_ManagedType->InvokeStaticMethod("Shutdown");
        }

        Plugin::Cleanup();
        m_Runtime.Reset();

        m_ImGui.Reset();
        m_Window.Reset();

        SDL_Quit();
        MIDI::Shutdown();

        if (m_OwnsLogger) {
            ResetLogger();
        }
    }

    void Application::Quit(int status) {
        m_Running = false;
        m_Status = status;
    }

    Application::Application(const std::vector<std::string>& arguments) {
        m_Running = false;
        m_Status = 0;

        m_Runtime = nullptr;
        m_ManagedType = nullptr;

        m_Executable = std::filesystem::absolute(arguments[0]).lexically_normal();
        auto executableDirectory = m_Executable.parent_path();

        std::filesystem::path resourceDir;
        std::optional<std::filesystem::path> logDir;
        if (executableDirectory.filename().string() == "bin") {
            resourceDir = executableDirectory / "../share/schmix";
        } else {
            resourceDir = std::filesystem::current_path() / "assets";
            logDir = std::filesystem::current_path() / "logs";
        }

        m_ResourceDirectory = resourceDir.lexically_normal();
        m_OwnsLogger = CreateLogger(logDir);

        // todo: dump build info
        SCHMIX_INFO("Schmix is still a work-in-progress. Report bugs at "
                    "https://github.com/heyallnorahere/schmix/issues/new");

        SCHMIX_INFO("Executable path: {}", m_Executable.string().c_str());
        SCHMIX_INFO("Resource directory: {}", m_ResourceDirectory.string().c_str());

        SCHMIX_INFO("Initializing...");

        MIDI::Init();

        if (!CreateWindow() || !InitImGui() || !InitRuntime()) {
            SCHMIX_ERROR("Initialization failed! Exiting 1...");
            Quit(1);
        }
    }

    bool Application::CreateWindow() {
        static const std::string title = "Schmix: shitass DAW";
        static constexpr std::uint32_t width = 1600;
        static constexpr std::uint32_t height = 900;

        if (!SDL_SetMemoryFunctions(Memory::Allocate, Memory::AllocateZeroedArray,
                                    Memory::Reallocate, Memory::Free)) {
            SCHMIX_ERROR("Failed to set SDL memory callbacks!");
            return false;
        }

        m_Window = Ref<Window>::Create(title, width, height);
        if (!m_Window->IsInitialized()) {
            SCHMIX_ERROR("Failed to create main window!");
            return false;
        }

        return true;
    }

    bool Application::InitImGui() {
        m_ImGui = Ref<ImGuiInstance>::Create(m_Window);
        if (!m_ImGui->IsInitialized()) {
            SCHMIX_ERROR("Failed to initialize Dear ImGui!");
            return false;
        }

        Window::SetEventCallback([this](const SDL_Event& event) { m_ImGui->ProcessEvent(event); });
        return true;
    }

    bool Application::InitRuntime() {
        m_Runtime = new ScriptRuntime(m_ResourceDirectory / "runtime");
        if (!m_Runtime->IsInitialized()) {
            SCHMIX_ERROR("Failed to initialize managed script runtime!");
            return false;
        }

        std::vector<ScriptBinding> bindings;
        Bindings::Get(bindings);
        m_Runtime->RegisterCoreBindings(bindings);

        if (!Plugin::Init(m_Runtime)) {
            SCHMIX_ERROR("Failed to initialize plugin interface!");
            return false;
        }

        if (!Plugin::LoadPlugins(m_ResourceDirectory / "plugins")) {
            SCHMIX_ERROR("Failed to load plugins!");
            return false;
        }

        auto& appType = m_Runtime->GetCore()->GetType("Schmix.UI.Application");
        if (!appType) {
            SCHMIX_ERROR("Failed to find Application type!");
            return false;
        }

        m_ManagedType = &appType;
        m_MIDIType = &m_Runtime->GetCore()->GetType("Schmix.Audio.MIDI");

        if (!appType.InvokeStaticMethod<bool>("Init")) {
            SCHMIX_ERROR("Failed to initialize Application in managed code!");
            return false;
        }

        return true;
    }

    void Application::Loop() {
        if (m_Status != 0) {
            return;
        }

        MIDI::Callbacks callbacks;
        callbacks.NoteBegin = std::bind(&Application::NoteBegin, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3);

        callbacks.NoteEnd =
            std::bind(&Application::NoteEnd, this, std::placeholders::_1, std::placeholders::_2);

        callbacks.ResetTime = std::bind(&Application::ResetTime, this);

        m_Running = true;
        while (m_Running) {
            Window::ProcessEvents();
            if (m_Window->IsCloseRequested()) {
                m_Running = false;
            }

            MIDI::Update(callbacks);

            m_ManagedType->InvokeStaticMethod("Update", (double)0);
        }
    }

    void Application::NoteBegin(const MIDI::NoteInfo& note, double velocity,
                                std::chrono::nanoseconds timeSinceLast) {
        m_MIDIType->InvokeStaticMethod("NoteBegin", &note, velocity, timeSinceLast.count());
    }

    void Application::NoteEnd(const MIDI::NoteInfo& note, std::chrono::nanoseconds timeSinceLast) {
        m_MIDIType->InvokeStaticMethod("NoteEnd", &note, timeSinceLast.count());
    }

    void Application::ResetTime() { m_MIDIType->InvokeStaticMethod("ResetTime"); }
} // namespace schmix
