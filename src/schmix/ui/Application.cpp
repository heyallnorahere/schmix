#include "schmixpch.h"
#include "schmix/ui/Application.h"

#include "schmix/script/ScriptRuntime.h"
#include "schmix/script/Bindings.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

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
        if (m_Window) {
            SDL_WaitForGPUIdle(m_Window->GetDevice());
        }

        delete m_Runtime;

        m_Mixer.Reset();

        if (m_Stream != nullptr) {
            SDL_DestroyAudioStream(m_Stream);
        }

        if (m_Context != nullptr) {
            SetImGuiContext();

            ImGui_ImplSDLGPU3_Shutdown();
            ImGui_ImplSDL3_Shutdown();

            ImGui::DestroyContext(m_Context);
        }

        m_Window.Reset();

        SDL_Quit();

        if (m_OwnsLogger) {
            ResetLogger();
        }
    }

    static void* ImGuiMemAlloc(std::size_t sz, void* user_data) { return Memory::Allocate(sz); }
    static void ImGuiMemFree(void* ptr, void* user_data) { return Memory::Free(ptr); }

    void Application::SetImGuiContext() const {
        ImGui::SetAllocatorFunctions(ImGuiMemAlloc, ImGuiMemFree);
        ImGui::SetCurrentContext(m_Context);
    }

    void Application::Quit(int status) {
        m_Running = false;
        m_Status = status;
    }

    Application::Application(const std::vector<std::string>& arguments) {
        m_Running = false;
        m_Status = 0;

        m_Stream = nullptr;

        m_Context = nullptr;

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
        SCHMIX_INFO("Hi!");
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

        Window::SetEventCallback([this](const SDL_Event& event) {
            SetImGuiContext();
            ImGui_ImplSDL3_ProcessEvent(&event);
        });

        return true;
    }

    bool Application::InitAudio() {
        static constexpr SDL_AudioDeviceID id = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            return false;
        }

        SDL_AudioSpec spec;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 2;
        spec.freq = 40960;

        m_Mixer = Ref<Mixer>::Create(spec.freq / 4, spec.freq, spec.channels);

        m_Stream = SDL_OpenAudioDeviceStream(id, &spec, nullptr, nullptr);
        if (!m_Stream) {
            SCHMIX_ERROR("Failed to open audio stream!");
            return false;
        }

        SDL_ResumeAudioStreamDevice(m_Stream);
        return true;
    }

    bool Application::InitImGui() {
        SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
        float displayScale = SDL_GetDisplayContentScale(primaryDisplay);

        IMGUI_CHECKVERSION();
        ImGui::SetAllocatorFunctions(ImGuiMemAlloc, ImGuiMemFree);

        m_Context = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_Context);

        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        (void)style;

        style.ScaleAllSizes(displayScale);
        style.FontScaleDpi *= displayScale;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.f;
            style.Colors[ImGuiCol_WindowBg].w = 1.f;
        }

        auto window = m_Window->GetWindow();
        auto device = m_Window->GetDevice();

        ImGui_ImplSDLGPU3_InitInfo initInfo{};
        initInfo.Device = device;
        initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
        initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        initInfo.SwapchainComposition = Window::SwapchainComposition;
        initInfo.PresentMode = Window::PresentMode;

        ImGui_ImplSDL3_InitForSDLGPU(window);
        ImGui_ImplSDLGPU3_Init(&initInfo);

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
        SetImGuiContext();

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        static bool showDemoWindow = true;
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(m_Window->GetDevice());
        SDL_GPUTexture* swapchainTexture = m_Window->AcquireImage(cmdBuffer);

        bool textureAcquired = swapchainTexture != nullptr;
        bool windowMinimized = drawData->DisplaySize.x <= 0.f || drawData->DisplaySize.y <= 0.f;

        if (textureAcquired && !windowMinimized) {
            ImGui_ImplSDLGPU3_PrepareDrawData(drawData, cmdBuffer);

            SDL_GPUColorTargetInfo targetInfo{};
            targetInfo.texture = swapchainTexture;
            targetInfo.clear_color = { 0.3f, 0.3f, 0.3f, 1.f };
            targetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            targetInfo.store_op = SDL_GPU_STOREOP_STORE;
            targetInfo.mip_level = 0;
            targetInfo.layer_or_depth_plane = 0;
            targetInfo.cycle = false;

            SDL_GPURenderPass* renderPass =
                SDL_BeginGPURenderPass(cmdBuffer, &targetInfo, 1, nullptr);

            ImGui_ImplSDLGPU3_RenderDrawData(drawData, cmdBuffer, renderPass);
            SDL_EndGPURenderPass(renderPass);
        }

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        SDL_SubmitGPUCommandBuffer(cmdBuffer);
    }

    void Application::ProcessAudio() {
        std::size_t chunkSize = m_Mixer->GetChunkSize();

        int queued = SDL_GetAudioStreamQueued(m_Stream);
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
                std::size_t channels = output.GetChannels();
                MonoSignal<float> streamData(chunkSize * channels);

                std::size_t totalSamples = streamData.GetLength();
                for (std::size_t i = 0; i < totalSamples; i++) {
                    std::size_t channelIndex = i % channels;
                    std::size_t sampleIndex = i / channels;

                    streamData[i] = (float)output[channelIndex][sampleIndex];
                }

                SDL_PutAudioStreamData(m_Stream, streamData.GetData(),
                                       totalSamples * sizeof(float));
            }
        }
    }
} // namespace schmix
