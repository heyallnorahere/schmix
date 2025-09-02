#pragma once
#include "schmix/core/Ref.h"

#include "schmix/core/SDL.h"

namespace schmix {
    class Window : public RefCounted {
    public:
        static constexpr SDL_GPUSwapchainComposition SwapchainComposition =
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR;

        static constexpr SDL_GPUPresentMode PresentMode = SDL_GPU_PRESENTMODE_VSYNC;

        using EventCallback = std::function<void(const SDL_Event&)>;

        static bool AddSubsystemReference();
        static void RemoveSubsystemReference();

        static void SetEventCallback(const EventCallback& callback);
        static void ProcessEvents();

        Window(const std::string& title, std::uint32_t width, std::uint32_t height);
        virtual ~Window() override;

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        void WaitForGPU() const;

        SDL_GPUTexture* AcquireImage(SDL_GPUCommandBuffer* cmdBuffer) const;

        SDL_Window* GetWindow() const { return m_Window; }
        SDL_GPUDevice* GetDevice() const { return m_Device; }

        bool IsCloseRequested() const { return m_CloseRequested; }
        bool IsInitialized() const { return m_Initialized; }

    private:
        bool m_HoldsReference;
        SDL_Window* m_Window;
        SDL_GPUDevice* m_Device;
        bool m_SwapchainCreated;

        bool m_CloseRequested;
        bool m_Initialized;
    };
} // namespace schmix
