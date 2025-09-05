#pragma once

namespace schmix {
    class MIDI {
    public:
        using Timestamp = std::chrono::steady_clock::time_point;

        struct NoteInfo {
            std::uint8_t ID, Channel;
        };

        struct Callbacks {
            std::function<void(const NoteInfo&, double, Timestamp)> NoteBegin;
            std::function<void(const NoteInfo&, Timestamp)> NoteEnd;
        };

        MIDI() = delete;

        static void Init();
        static void Shutdown();

        static void Update(const Callbacks& callbacks = {});

        static Timestamp Now();
    };
} // namespace schmix
