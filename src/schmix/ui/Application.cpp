#include "schmixpch.h"
#include "schmix/ui/Application.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

namespace schmix {
    Application* s_App;

    int Application::Run(int argc, const char** argv) {
        if (s_App != nullptr) {
            return -1;
        }

        s_App = new Application;
        s_App->Loop();

        int status = s_App->m_Status;

        delete s_App;
        s_App = nullptr;

        return status;
    }

    Application& Application::Get() { return *s_App; }

    Application::~Application() {
        if (m_Device != nullptr) {
            SDL_WaitForGPUIdle(m_Device);
        }

        if (m_Context != nullptr) {
            ImGui::SetCurrentContext(m_Context);

            ImGui_ImplSDLGPU3_Shutdown();
            ImGui_ImplSDL3_Shutdown();

            ImGui::DestroyContext(m_Context);
        }

        if (m_SwapchainCreated) {
            SDL_ReleaseWindowFromGPUDevice(m_Device, m_Window);
        }

        if (m_Device != nullptr) {
            SDL_DestroyGPUDevice(m_Device);
        }

        if (m_Window != nullptr) {
            SDL_DestroyWindow(m_Window);
        }

        SDL_Quit();
    }

    void Application::Quit(int status) {
        m_Running = false;
        m_Status = status;
    }

    Application::Application() {
        m_Running = false;
        m_Status = 0;

        m_Window = nullptr;
        m_Device = nullptr;
        m_SwapchainCreated = false;

        m_Context = nullptr;

        if (!CreateWindow() || !InitImGui()) {
            Quit(1);
        }
    }

    static constexpr SDL_GPUSwapchainComposition s_SwapchainComposition =
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR;

    static constexpr SDL_GPUPresentMode s_PresentMode = SDL_GPU_PRESENTMODE_VSYNC;

    bool Application::CreateWindow() {
        static const std::string title = "Schmix: shitass DAW";
        static constexpr std::uint32_t width = 1600;
        static constexpr std::uint32_t height = 900;

        static constexpr SDL_WindowFlags flags =
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

        static constexpr SDL_GPUShaderFormat shaderFormat =
            SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB;

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
            return false;
        }

        m_Window = SDL_CreateWindow(title.c_str(), (int)width, (int)height, flags);
        if (m_Window == nullptr) {
            return false;
        }

        SDL_ShowWindow(m_Window);

        m_Device = SDL_CreateGPUDevice(shaderFormat, true, nullptr);
        if (m_Device == nullptr) {
            return false;
        }

        if (!SDL_ClaimWindowForGPUDevice(m_Device, m_Window)) {
            return false;
        }

        m_SwapchainCreated = true;

        SDL_SetGPUSwapchainParameters(m_Device, m_Window, s_SwapchainComposition, s_PresentMode);

        return true;
    }

    bool Application::InitImGui() {
        SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
        float displayScale = SDL_GetDisplayContentScale(primaryDisplay);

        IMGUI_CHECKVERSION();

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

        ImGui_ImplSDLGPU3_InitInfo initInfo{};
        initInfo.Device = m_Device;
        initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(m_Device, m_Window);
        initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        initInfo.SwapchainComposition = s_SwapchainComposition;
        initInfo.PresentMode = s_PresentMode;

        ImGui_ImplSDL3_InitForSDLGPU(m_Window);
        ImGui_ImplSDLGPU3_Init(&initInfo);

        return true;
    }

    void Application::Loop() {
        if (m_Status != 0) {
            return;
        }

        m_Running = true;
        while (m_Running) {
            Render();
            ProcessEvents();
        }
    }

    void Application::Render() {
        ImGui::SetCurrentContext(m_Context);

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        static bool showDemoWindow = true;
        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(m_Device);

        SDL_GPUTexture* swapchainTexture;
        SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuffer, m_Window, &swapchainTexture, nullptr,
                                              nullptr);

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

    void Application::ProcessEvents() {
        SDL_WindowID appWindowID = SDL_GetWindowID(m_Window);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            std::uint32_t eventClass = (((std::uint32_t)event.type) >> 8) & 0xFF;
            if (eventClass == 0x02 && event.window.windowID != appWindowID) {
                // window event not pertaining to ours. skip
                continue;
            }

            switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                Quit();
                break;
            }
        }
    }
} // namespace schmix
