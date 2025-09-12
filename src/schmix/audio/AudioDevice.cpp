#include "schmixpch.h"
#include "schmix/audio/AudioDevice.h"

#include "schmix/core/SDL.h"

namespace schmix {
    static std::uint32_t s_SubsystemReferences = 0;
    static constexpr SDL_InitFlags s_AudioSubsystem = SDL_INIT_AUDIO;

    std::size_t AudioDevice::GetDefaultInputID() { return SDL_AUDIO_DEVICE_DEFAULT_RECORDING; }
    std::size_t AudioDevice::GetDefaultOutputID() { return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK; }

    std::vector<std::size_t> AudioDevice::GetInputDevices() {
        if (!AddSubsystemReference()) {
            SCHMIX_ERROR("Failed to add ref to audio subsystem; 0 input devices found");
            return {};
        }

        int count;
        SDL_AudioDeviceID* ids = SDL_GetAudioRecordingDevices(&count);

        if (ids == nullptr) {
            SCHMIX_ERROR("Failed to query SDL recording devices: {}", SDL_GetError());

            RemoveSubsystemReference();
            return {};
        }

        std::vector<std::size_t> devices;
        for (int i = 0; i < count; i++) {
            auto id = (std::size_t)ids[i];
            devices.push_back(id);
        }

        RemoveSubsystemReference();
        return devices;
    }

    std::vector<std::size_t> AudioDevice::GetOutputDevices() {
        if (!AddSubsystemReference()) {
            SCHMIX_ERROR("Failed to add ref to audio subsystem; 0 output devices found");
            return {};
        }

        int count;
        SDL_AudioDeviceID* ids = SDL_GetAudioPlaybackDevices(&count);

        if (ids == nullptr) {
            SCHMIX_ERROR("Failed to query SDL playback devices: {}", SDL_GetError());

            RemoveSubsystemReference();
            return {};
        }

        std::vector<std::size_t> devices;
        for (int i = 0; i < count; i++) {
            auto id = (std::size_t)ids[i];
            devices.push_back(id);
        }

        SDL_free(ids);
        RemoveSubsystemReference();

        return devices;
    }

    std::optional<std::string> AudioDevice::GetDeviceName(std::size_t id) {
        if (!AddSubsystemReference()) {
            SCHMIX_ERROR("Failed to add ref to audio subsystem; failed to get device name");
            return {};
        }

        auto deviceID = (SDL_AudioDeviceID)id;
        auto name = SDL_GetAudioDeviceName(deviceID);

        RemoveSubsystemReference();
        if (name == nullptr) {
            SCHMIX_ERROR("Failed to retrieve device name: {}", SDL_GetError());
            return {};
        }

        return name;
    }

    bool AudioDevice::AddSubsystemReference() {
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

    void AudioDevice::RemoveSubsystemReference() {
        if (s_SubsystemReferences == 0) {
            SCHMIX_WARN("Audio SDL subsystems have no references; skipping remove");
            return;
        }

        if (--s_SubsystemReferences == 0) {
            SCHMIX_DEBUG("Quitting audio SDL subsystems...");
            SDL_QuitSubSystem(s_AudioSubsystem);
        }
    }

    AudioDevice::AudioDevice(std::size_t deviceID, std::size_t sampleRate, std::size_t channels) {
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

        SCHMIX_DEBUG("Opening audio stream of device with ID: {}", m_DeviceID);

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

    AudioDevice::~AudioDevice() {
        if (m_Stream != nullptr) {
            SDL_DestroyAudioStream(m_Stream);
        }

        if (m_HoldsReference) {
            RemoveSubsystemReference();
        }
    }

    std::optional<std::size_t> AudioDevice::GetInterleavedAudio(float* samples,
                                                                std::size_t countRequested) {
        if (!m_Initialized) {
            SCHMIX_WARN("Audio output not initialized; skipping audio push");
            return {};
        }

        std::size_t offset = 0;
        while (offset < countRequested) {
            std::size_t bytesRemaining = countRequested - offset;
            int bytes = SDL_GetAudioStreamData(m_Stream, &samples[offset],
                                               bytesRemaining * m_Channels * sizeof(float));

            if (bytes < 0) {
                SCHMIX_ERROR("Failed to retrieve data from SDL audio stream!");
                return {};
            }

            offset += bytes;
            if (bytes == 0) {
                break;
            }
        }

        return offset / (m_Channels * sizeof(float));
    }

    bool AudioDevice::PutInterleavedAudio(const float* samples, std::size_t count) {
        if (!m_Initialized) {
            SCHMIX_WARN("Audio output not initialized; skipping audio push");
            return false;
        }

        if (!SDL_PutAudioStreamData(m_Stream, samples, count * m_Channels * sizeof(float))) {
            SCHMIX_ERROR("Failed to push audio: {}", SDL_GetError());
            return false;
        }

        return true;
    }

    bool AudioDevice::Flush() { return SDL_FlushAudioStream(m_Stream); }

    std::size_t AudioDevice::GetAvailableSamples() const {
        if (!m_Initialized) {
            SCHMIX_WARN("Attempted to query stream queue on uninitialized output; returning 0");
            return 0;
        }

        int bytes = SDL_GetAudioStreamAvailable(m_Stream);
        return (std::size_t)bytes / (m_Channels * sizeof(float));
    }

    std::size_t AudioDevice::GetQueuedSamples() const {
        if (!m_Initialized) {
            SCHMIX_WARN("Attempted to query stream queue on uninitialized output; returning 0");
            return 0;
        }

        int bytes = SDL_GetAudioStreamQueued(m_Stream);
        return (std::size_t)bytes / (m_Channels * sizeof(float));
    }
} // namespace schmix
