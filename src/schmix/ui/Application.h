#pragma once

#include "schmix/audio/Mixer.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_GPUDevice SDL_GPUDevice;
typedef struct SDL_AudioStream SDL_AudioStream;

struct ImGuiContext;

namespace schmix {
    class Application {
    public:
        static int Run(int argc, const char** argv);

        static Application& Get();

        ~Application();

        SDL_Window* GetWindow() const { return m_Window; }
        ImGuiContext* GetImGuiContext() const { return m_Context; }

        void SetImGuiContext() const;

        void Quit(int status = 0);

    private:
        Application();

        bool CreateWindow();
        bool InitAudio();
        bool InitImGui();
        bool InitRuntime();

        void Loop();
        void Render();
        void ProcessAudio();
        void ProcessEvents();

        bool m_Running;
        int m_Status;

        SDL_Window* m_Window;
        SDL_GPUDevice* m_Device;
        bool m_SwapchainCreated;

        Ref<Mixer> m_Mixer;
        SDL_AudioStream* m_Stream;

        ImGuiContext* m_Context;

        ScriptRuntime* m_Runtime;
    };
} // namespace schmix
