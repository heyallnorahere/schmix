#pragma once

#include "schmix/audio/AudioOutput.h"
#include "schmix/audio/Mixer.h"

#include "schmix/ui/Window.h"

typedef struct SDL_AudioStream SDL_AudioStream;

struct ImGuiContext;

namespace schmix {
    class ScriptRuntime;

    class Application {
    public:
        static int Run(int argc, const char** argv);

        static Application& Get();

        ~Application();

        const Ref<Window>& GetWindow() const { return m_Window; }
        ImGuiContext* GetImGuiContext() const { return m_Context; }

        void SetImGuiContext() const;

        void Quit(int status = 0);

    private:
        Application(const std::vector<std::string>& arguments);

        bool CreateWindow();
        bool InitAudio();
        bool InitImGui();
        bool InitRuntime();

        void Loop();
        void Render();
        void ProcessAudio();

        std::filesystem::path m_Executable;
        std::filesystem::path m_ResourceDirectory;

        bool m_Running;
        int m_Status;

        bool m_OwnsLogger;

        Ref<Window> m_Window;

        Ref<Mixer> m_Mixer;
        Ref<AudioOutput> m_Output;

        ImGuiContext* m_Context;

        ScriptRuntime* m_Runtime;
    };
} // namespace schmix
