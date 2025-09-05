#pragma once

#include "schmix/audio/MIDI.h"

#include "schmix/script/ScriptRuntime.h"

#include "schmix/ui/ImGuiInstance.h"
#include "schmix/ui/Window.h"

typedef struct SDL_AudioStream SDL_AudioStream;

struct ImGuiContext;

namespace schmix {
    class Application {
    public:
        static int Run(int argc, const char** argv);

        static Application& Get();

        ~Application();

        const Ref<Window>& GetWindow() const { return m_Window; }
        const Ref<ImGuiInstance>& GetImGuiInstance() const { return m_ImGui; }

        bool IsRunning() const { return m_Running; }

        void Quit(int status = 0);

    private:
        Application(const std::vector<std::string>& arguments);

        bool CreateWindow();
        bool InitImGui();
        bool InitRuntime();

        void Loop();

        void NoteBegin(const MIDI::NoteInfo& note, double velocity,
                       std::chrono::nanoseconds timeSinceLast);

        void NoteEnd(const MIDI::NoteInfo& info, std::chrono::nanoseconds timeSinceLast);

        void ResetTime();

        std::filesystem::path m_Executable;
        std::filesystem::path m_ResourceDirectory;

        bool m_Running;
        int m_Status;

        bool m_OwnsLogger;

        Ref<Window> m_Window;

        Ref<ImGuiInstance> m_ImGui;

        Ref<ScriptRuntime> m_Runtime;
        Coral::Type* m_ManagedType;

        Coral::Type* m_MIDIType;
    };
} // namespace schmix
