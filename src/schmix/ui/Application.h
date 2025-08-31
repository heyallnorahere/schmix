#pragma once

typedef struct SDL_Window SDL_Window;
typedef struct SDL_GPUDevice SDL_GPUDevice;

struct ImGuiContext;

namespace schmix {
    class Application {
    public:
        static int Run(int argc, const char** argv);

        static Application& Get();

        ~Application();

        SDL_Window* GetWindow() const { return m_Window; }
        ImGuiContext* GetImGuiContext() const { return m_Context; }

        void Quit(int status = 0);

    private:
        Application();

        bool CreateWindow();
        bool InitImGui();

        void Loop();
        void Render();
        void ProcessEvents();

        bool m_Running;
        int m_Status;

        SDL_Window* m_Window;
        SDL_GPUDevice* m_Device;
        bool m_SwapchainCreated;

        ImGuiContext* m_Context;
    };
} // namespace schmix
