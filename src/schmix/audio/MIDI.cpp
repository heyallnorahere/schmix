#include "schmixpch.h"
#include "schmix/audio/MIDI.h"

#include <libremidi/libremidi.hpp>

#include <mutex>
#include <queue>

namespace schmix {
    struct MIDIData {
        std::mutex Lock;
        std::queue<libremidi::message> MessageQueue;

        std::unique_ptr<libremidi::observer> Observer;
        std::unordered_map<libremidi::device_identifier, std::unique_ptr<libremidi::midi_in>> In;
    };

    static std::unique_ptr<MIDIData> s_Data;

    using MIDIDuration = std::chrono::duration<libremidi::timestamp, std::nano>;
    static libremidi::timestamp GetTimestamp(libremidi::timestamp reference) {
        auto now = MIDI::Now();
        auto fromEpoch = now.time_since_epoch();

        return std::chrono::duration_cast<MIDIDuration>(fromEpoch).count();
    }

    static void MIDILog(spdlog::level::level_enum level, std::string_view errorText,
                        const libremidi::source_location& loc) {
        spdlog::source_loc location;
        location.filename = loc.file_name();
        location.funcname = loc.function_name();
        location.line = loc.line();

        g_Logger->log(location, level, errorText.data());
    }

    static void MIDIError(std::string_view errorText, const libremidi::source_location& loc) {
        MIDILog(spdlog::level::err, errorText, loc);
    }

    static void MIDIWarn(std::string_view errorText, const libremidi::source_location& loc) {
        MIDILog(spdlog::level::warn, errorText, loc);
    }

    static void OnMessage(libremidi::message&& message) {
        SCHMIX_DEBUG("Message received");

        std::lock_guard lock(s_Data->Lock);
        s_Data->MessageQueue.push(message);
    }

    static void InputAdded(const libremidi::input_port& port) {
        SCHMIX_INFO("MIDI device added: {}", port.display_name);

        libremidi::input_configuration config;
        config.timestamps = libremidi::timestamp_mode::Custom;
        config.get_timestamp = GetTimestamp;

        config.on_error = MIDIError;
        config.on_warning = MIDIWarn;

        config.on_message = OnMessage;

        auto in = std::make_unique<libremidi::midi_in>(
            config, libremidi::midi2::in_default_configuration());

        auto error = in->open_port(port);
        if (error != stdx::error{}) {
            error.throw_exception();
        }

        std::lock_guard lock(s_Data->Lock);
        s_Data->In[port.device] = std::move(in);
    }

    static void InputRemoved(const libremidi::input_port& port) {
        SCHMIX_INFO("MIDI device removed: {}", port.display_name);

        std::lock_guard lock(s_Data->Lock);
        s_Data->In.erase(port.device);
    }

    void MIDI::Init() {
        if (s_Data) {
            return;
        }

        SCHMIX_DEBUG("Initializing MIDI...");

        libremidi::observer_configuration config;
        config.input_added = InputAdded;
        config.input_removed = InputRemoved;

        config.on_error = MIDIError;
        config.on_warning = MIDIWarn;

        s_Data = std::make_unique<MIDIData>();
        s_Data->Observer = std::make_unique<libremidi::observer>(
            config, libremidi::midi2::observer_default_configuration());
    }

    void MIDI::Shutdown() {
        if (!s_Data) {
            return;
        }

        SCHMIX_DEBUG("Shutting down MIDI...");

        s_Data->In.clear();
        s_Data->Observer.reset();

        s_Data.reset();
    }

    static MIDI::NoteInfo NoteFromMessage(const libremidi::message& message) {
        MIDI::NoteInfo note;
        note.Channel = message[0] & 0xF;
        note.ID = message[1];

        return note;
    }

    static MIDI::Timestamp ConvertTimestamp(libremidi::timestamp ts) {
        auto midiTimeSinceEpoch = MIDIDuration(ts);
        auto timeSinceEpoch =
            std::chrono::duration_cast<MIDI::Timestamp::duration>(midiTimeSinceEpoch);

        return MIDI::Timestamp(timeSinceEpoch);
    }

    void MIDI::Update(const Callbacks& callbacks) {
        std::lock_guard lock(s_Data->Lock);

        while (!s_Data->MessageQueue.empty()) {
            const auto& message = s_Data->MessageQueue.front();

            auto type = message.get_message_type();
            auto timestamp = ConvertTimestamp(message.timestamp);

            switch (type) {
            case libremidi::message_type::NOTE_ON: {
                auto note = NoteFromMessage(message);

                std::uint8_t velocityByte = message[2];
                double velocity =
                    (double)velocityByte * 2 / (double)std::numeric_limits<std::uint8_t>::max();

                if (callbacks.NoteBegin) {
                    callbacks.NoteBegin(note, velocity, timestamp);
                } else {
                    SCHMIX_DEBUG("Note {} pressed on channel {} with velocity {}", note.ID,
                                 note.Channel, velocity);
                }
            } break;
            case libremidi::message_type::NOTE_OFF: {
                auto note = NoteFromMessage(message);

                if (callbacks.NoteEnd) {
                    callbacks.NoteEnd(note, timestamp);
                } else {
                    SCHMIX_DEBUG("Note {} released on channel {}", note.ID, note.Channel);
                }
            } break;
            default:
                SCHMIX_DEBUG("Unimplemented message; skipping");
                break;
            }

            s_Data->MessageQueue.pop();
        }
    }

    MIDI::Timestamp MIDI::Now() { return std::chrono::steady_clock::now(); }
} // namespace schmix
