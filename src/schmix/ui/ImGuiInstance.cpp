#include "schmixpch.h"
#include "schmix/ui/ImGuiInstance.h"

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

#include <imnodes.h>

namespace schmix {
    void* ImGuiInstance::MemAlloc(std::size_t sz, void* user_data) { return Memory::Allocate(sz); }
    void ImGuiInstance::MemFree(void* ptr, void* user_data) { return Memory::Free(ptr); }

    ImGuiInstance::ImGuiInstance(const Ref<Window>& window) {
        m_Window = window;

        m_Context = nullptr;
        m_NodesContext = nullptr;

        m_Initialized = false;

        m_PlatformInitialized = false;
        m_RendererInitialized = false;

        if (!m_Window->IsInitialized()) {
            SCHMIX_ERROR("Passed window is not initialized!");
            return;
        }

        SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
        float displayScale = SDL_GetDisplayContentScale(primaryDisplay);

        IMGUI_CHECKVERSION();
        ImGui::SetAllocatorFunctions(MemAlloc, MemFree);

        SCHMIX_INFO("Initializing ImGui version {}", IMGUI_VERSION);

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

        auto sdlWindow = m_Window->GetWindow();
        if (!ImGui_ImplSDL3_InitForSDLGPU(sdlWindow)) {
            SCHMIX_ERROR("Failed to initialize ImGui platform backend for SDL!");
            return;
        }

        m_PlatformInitialized = true;

        auto device = m_Window->GetDevice();
        ImGui_ImplSDLGPU3_InitInfo initInfo{};

        initInfo.Device = device;
        initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, sdlWindow);
        initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
        initInfo.SwapchainComposition = Window::SwapchainComposition;
        initInfo.PresentMode = Window::PresentMode;

        if (!ImGui_ImplSDLGPU3_Init(&initInfo)) {
            SCHMIX_ERROR("Failed to initialize ImGui GPU renderer backend for SDL!");
            return;
        }

        m_RendererInitialized = true;

        m_NodesContext = imnodes::CreateContext();
        imnodes::SetCurrentContext(m_NodesContext);

        m_Initialized = true;
    }

    ImGuiInstance::~ImGuiInstance() {
        MakeContextCurrent();
        
        if (m_NodesContext != nullptr) {
            imnodes::DestroyContext(m_NodesContext);
            imnodes::SetCurrentContext(nullptr);
        }

        if (m_RendererInitialized) {
            m_Window->WaitForGPU();
            ImGui_ImplSDLGPU3_Shutdown();
        }

        if (m_PlatformInitialized) {
            ImGui_ImplSDL3_Shutdown();
        }

        if (m_Context != nullptr) {
            ImGui::DestroyContext(m_Context);
            ImGui::SetCurrentContext(nullptr);
        }
    }

    void ImGuiInstance::ProcessEvent(const SDL_Event& event) {
        if (!m_Initialized) {
            SCHMIX_WARN("ImGui instance not initialized; skipping event");
            return;
        }

        MakeContextCurrent();
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    void ImGuiInstance::MakeContextCurrent() const {
        if (!m_Initialized) {
            SCHMIX_WARN("Attempted to use an uninitialized ImGui instance; returning");
            return;
        }

        ImGui::SetAllocatorFunctions(MemAlloc, MemFree);
        ImGui::SetCurrentContext(m_Context);

        imnodes::SetCurrentContext(m_NodesContext);
    }

    bool ImGuiInstance::NewFrame() {
        if (!m_Initialized) {
            SCHMIX_WARN("Attempted to call NewFrame on an uninitialized ImGui instance; skipping");
            return false;
        }

        MakeContextCurrent();

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        return true;
    }

    bool ImGuiInstance::RenderAndPresent() {
        if (!m_Initialized) {
            SCHMIX_WARN("Attempted to render an uninitialized ImGui instance; skipping");
            return false;
        }

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(m_Window->GetDevice());
        if (cmdBuffer == nullptr) {
            SCHMIX_ERROR("Failed to acquire command buffer for rendering: {}", SDL_GetError());
            return false;
        }

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

        if (!SDL_SubmitGPUCommandBuffer(cmdBuffer)) {
            SCHMIX_ERROR("Failed to submit command buffer for rendering: {}", SDL_GetError());
            return false;
        }

        return true;
    }
} // namespace schmix
