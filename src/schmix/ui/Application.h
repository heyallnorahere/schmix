#pragma once

#include "schmix/ui/ImGuiInstance.h"
#include "schmix/ui/Window.h"

#include "schmix/script/ScriptRuntime.h"

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
        const Ref<ImGuiInstance>& GetImGuiInstance() const { return m_ImGui; }

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

        Ref<ImGuiInstance> m_ImGui;

        Ref<ScriptRuntime> m_Runtime;
        std::unique_ptr<Coral::ManagedObject> m_Instance;
    };
} // namespace schmix
