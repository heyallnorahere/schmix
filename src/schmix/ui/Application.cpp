#include "schmixpch.h"
#include "schmix/ui/Application.h"

#include "schmix/script/ScriptRuntime.h"
#include "schmix/script/Bindings.h"

#include <imgui.h>

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
        delete m_Runtime;

        m_Mixer.Reset();
        m_Output.Reset();

        m_ImGui.Reset();
        m_Window.Reset();

        SDL_Quit();

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

        if (!CreateWindow() || !InitAudio() || !InitImGui() || !InitRuntime()) {
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

    bool Application::InitAudio() {
        static constexpr std::size_t sampleRate = 40960;
        static constexpr std::size_t channels = 2;

        m_Mixer = Ref<Mixer>::Create(sampleRate / 4, sampleRate, channels);

        std::uint32_t deviceID = WindowAudioOutput::GetDefaultDeviceID();
        m_Output = Ref<WindowAudioOutput>::Create(deviceID, sampleRate, channels);

        if (!m_Output->IsInitialized()) {
            SCHMIX_ERROR("Failed to open audio output!");
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

        return true;
    }

    void Application::Loop() {
        if (m_Status != 0) {
            return;
        }

        m_Running = true;
        while (m_Running) {
            Render();
            ProcessAudio();

            Window::ProcessEvents();
            if (m_Window->IsCloseRequested()) {
                m_Running = false;
            }
        }
    }

    void Application::Render() {
        m_ImGui->NewFrame();

        static bool showDemoWindow = true;
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        m_ImGui->RenderAndPresent();
    }

    void Application::ProcessAudio() {
        std::size_t chunkSize = m_Mixer->GetChunkSize();

        std::size_t queued = m_Output->GetQueuedSamples();
        if (queued < chunkSize) {
            m_Mixer->Reset();

            {
                auto core = m_Runtime->GetCore();
                auto& test = core->GetType("Schmix.Test");

                test.InvokeStaticMethod("AddSineSignal_Native", (double)440, m_Mixer.Raw(),
                                        (std::uint32_t)0);
            }

            Mixer::Signal output = m_Mixer->EvaluateChannel(0);
            if (output) {
                m_Output->PutAudio(output);
            }
        }
    }
} // namespace schmix
