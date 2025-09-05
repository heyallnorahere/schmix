#pragma once

namespace schmix {
    class MIDI {
    public:
        struct NoteInfo {
            std::uint8_t ID, Channel;
        };

        struct Callbacks {
            std::function<void(const NoteInfo&, double, std::chrono::nanoseconds)> NoteBegin;
            std::function<void(const NoteInfo&, std::chrono::nanoseconds)> NoteEnd;

            std::function<void()> ResetTime;
        };

        MIDI() = delete;

        static void Init();
        static void Shutdown();

        static void Update(const Callbacks& callbacks = {});
    };
} // namespace schmix
