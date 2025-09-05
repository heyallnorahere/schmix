#include "schmixpch.h"
#include "schmix/audio/OutputDevice.h"

#include "schmix/core/SDL.h"

namespace schmix {
    static std::uint32_t s_SubsystemReferences = 0;
    static constexpr SDL_InitFlags s_AudioSubsystem = SDL_INIT_AUDIO;

    std::size_t OutputDevice::GetDefaultDeviceID() {
        return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    }

    bool OutputDevice::AddSubsystemReference() {
        if (s_SubsystemReferences == 0) {
            SCHMIX_DEBUG("Initializing audio SDL subsystems...");

            if (!SDL_InitSubSystem(s_AudioSubsystem)) {
                SCHMIX_ERROR("Failed to initialize SDL subsystems: {}", SDL_GetError());
                return false;
            }
        }

        s_SubsystemReferences++;
        return true;
    }

    void OutputDevice::RemoveSubsystemReference() {
        if (s_SubsystemReferences == 0) {
            SCHMIX_WARN("Audio SDL subsystems have no references; skipping remove");
            return;
        }

        if (--s_SubsystemReferences == 0) {
            SCHMIX_DEBUG("Quitting audio SDL subsystems...");
            SDL_QuitSubSystem(s_AudioSubsystem);
        }
    }

    OutputDevice::OutputDevice(std::size_t deviceID, std::size_t sampleRate,
                                         std::size_t channels) {
        m_DeviceID = deviceID;

        m_SampleRate = sampleRate;
        m_Channels = channels;

        m_HoldsReference = false;
        m_Initialized = false;
        m_Stream = nullptr;

        if (!AddSubsystemReference()) {
            SCHMIX_ERROR("Failed to add reference to audio subsystems!");
            return;
        }

        m_HoldsReference = true;

        SDL_AudioSpec spec;
        spec.format = SDL_AUDIO_F32;
        spec.channels = (int)channels;
        spec.freq = (int)sampleRate;

        SCHMIX_DEBUG("Opening audio output stream of device with ID: {}", m_DeviceID);

        m_Stream = SDL_OpenAudioDeviceStream((SDL_AudioDeviceID)deviceID, &spec, nullptr, nullptr);
        if (!m_Stream) {
            SCHMIX_ERROR("Failed to open audio stream!");
            return;
        }

        if (!SDL_ResumeAudioStreamDevice(m_Stream)) {
            SCHMIX_ERROR("Failed to resume audio device!");
            return;
        }

        m_Initialized = true;
    }

    OutputDevice::~OutputDevice() {
        if (m_Stream != nullptr) {
            SDL_DestroyAudioStream(m_Stream);
        }

        if (m_HoldsReference) {
            RemoveSubsystemReference();
        }
    }

    bool OutputDevice::PutInterleavedAudio(const MonoSignal<float>& interleaved) {
        if (!m_Initialized) {
            SCHMIX_WARN("Audio output not initialized; skipping audio push");
            return false;
        }

        if (!SDL_PutAudioStreamData(m_Stream, interleaved.GetData(),
                                    interleaved.GetLength() * sizeof(float))) {
            SCHMIX_ERROR("Failed to push audio: {}", SDL_GetError());
            return false;
        }

        return true;
    }

    std::size_t OutputDevice::GetQueuedSamples() const {
        if (!m_Initialized) {
            SCHMIX_WARN("Attempted to query stream queue on uninitialized output; returning 0");
            return 0;
        }

        return (std::size_t)SDL_GetAudioStreamQueued(m_Stream);
    }
} // namespace schmix
