#include "schmixpch.h"
#include "schmix/ui/Window.h"

namespace schmix {
    static std::uint32_t s_SubsystemReferences = 0;
    static constexpr SDL_InitFlags s_WindowSubsystems = SDL_INIT_VIDEO | SDL_INIT_GAMEPAD;

    static Window::EventCallback s_WindowEventCallback = nullptr;

    // imgui creates windows for viewports. we need to keep track of which ones are ours
    static std::unordered_map<SDL_WindowID, Window*> s_SchmixWindows;

    bool Window::AddSubsystemReference() {
        if (s_SubsystemReferences == 0) {
            SCHMIX_DEBUG("Initializing window SDL subsystems...");

            if (!SDL_InitSubSystem(s_WindowSubsystems)) {
                SCHMIX_ERROR("Failed to initialize SDL subsystems: {}", SDL_GetError());
                return false;
            }
        }

        s_SubsystemReferences++;
        return true;
    }

    void Window::RemoveSubsystemReference() {
        if (s_SubsystemReferences == 0) {
            SCHMIX_WARN("Window SDL subsystems have no references; skipping remove");
            return;
        }

        if (--s_SubsystemReferences == 0) {
            SCHMIX_DEBUG("Quitting window SDL subsystems...");
            SDL_QuitSubSystem(s_WindowSubsystems);
        }
    }

    void Window::SetEventCallback(const EventCallback& callback) {
        s_WindowEventCallback = callback;
    }

    void Window::ProcessEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (s_WindowEventCallback) {
                s_WindowEventCallback(event);
            }

            std::uint32_t eventClass = (((std::uint32_t)event.type) >> 8) & 0xFF;
            if (eventClass == 0x02 && !s_SchmixWindows.contains(event.window.windowID)) {
                // window event not pertaining to ours. skip
                continue;
            }

            switch (event.type) {
            case SDL_EVENT_QUIT:
                for (const auto& [id, window] : s_SchmixWindows) {
                    window->m_CloseRequested = true;
                }

                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                s_SchmixWindows.at(event.window.windowID)->m_CloseRequested = true;
                break;
            }
        }
    }

    Window::Window(const std::string& title, std::uint32_t width, std::uint32_t height) {
        static constexpr SDL_WindowFlags flags =
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

        static constexpr SDL_GPUShaderFormat shaderFormat =
            SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB;

        m_CloseRequested = false;
        m_Initialized = false;

        m_HoldsReference = false;
        m_Window = nullptr;
        m_Device = nullptr;
        m_SwapchainCreated = false;

        if (!AddSubsystemReference()) {
            SCHMIX_ERROR("Failed to add window subsystem reference; aborting window creation");
            return;
        }

        m_HoldsReference = true;

        m_Window = SDL_CreateWindow(title.c_str(), (int)width, (int)height, flags);
        if (m_Window == nullptr) {
            SCHMIX_ERROR("Failed to create SDL window: {}", SDL_GetError());
            return;
        }

        SDL_ShowWindow(m_Window);

        m_Device = SDL_CreateGPUDevice(shaderFormat, true, nullptr);
        if (m_Device == nullptr) {
            SCHMIX_ERROR("Failed to acquire SDL graphics device: {}", SDL_GetError());
            return;
        }

        if (!SDL_ClaimWindowForGPUDevice(m_Device, m_Window)) {
            SCHMIX_ERROR("Failed to create window swapchain: {}", SDL_GetError());
            return;
        }

        m_SwapchainCreated = true;

        if (!SDL_SetGPUSwapchainParameters(m_Device, m_Window, SwapchainComposition, PresentMode)) {
            SCHMIX_ERROR("Failed to configure window swapchain: {}", SDL_GetError());
            return;
        }

        SDL_WindowID id = SDL_GetWindowID(m_Window);
        s_SchmixWindows[id] = this;

        m_Initialized = true;
    }

    Window::~Window() {
        if (m_SwapchainCreated) {
            SDL_ReleaseWindowFromGPUDevice(m_Device, m_Window);
        }

        if (m_Device != nullptr) {
            SDL_DestroyGPUDevice(m_Device);
        }

        if (m_Window != nullptr) {
            SDL_WindowID id = SDL_GetWindowID(m_Window);
            s_SchmixWindows.erase(id);

            SDL_DestroyWindow(m_Window);
        }

        if (m_HoldsReference) {
            RemoveSubsystemReference();
        }
    }

    void Window::WaitForGPU() const { SDL_WaitForGPUIdle(m_Device); }

    SDL_GPUTexture* Window::AcquireImage(SDL_GPUCommandBuffer* cmdBuffer) const {
        SDL_GPUTexture* swapchainTexture;
        SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuffer, m_Window, &swapchainTexture, nullptr,
                                              nullptr);

        return swapchainTexture;
    }
} // namespace schmix
