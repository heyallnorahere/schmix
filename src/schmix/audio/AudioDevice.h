#pragma once
#include "schmix/core/Ref.h"

#include "schmix/audio/Signal.h"

typedef struct SDL_AudioStream SDL_AudioStream;

namespace schmix {
    class AudioDevice : public RefCounted {
    public:
        template <typename _Ty>
        static constexpr float ConvertSample(_Ty sample) {
            if constexpr (std::is_integral_v<_Ty>) {
                constexpr _Ty max = std::numeric_limits<_Ty>::max();
                return static_cast<float>((float)sample / (float)max);
            } else {
                return static_cast<float>(sample);
            }
        }

        template <typename _Ty>
        static constexpr _Ty ConvertSampleBack(float sample) {
            if constexpr (std::is_integral_v<_Ty>) {
                constexpr _Ty max = std::numeric_limits<_Ty>::max();
                return static_cast<_Ty>(sample * max);
            } else {
                return static_cast<_Ty>(sample);
            }
        }

        static std::size_t GetDummyID();
        static std::size_t GetDefaultInputID();
        static std::size_t GetDefaultOutputID();

        static std::vector<std::size_t> GetInputDevices();
        static std::vector<std::size_t> GetOutputDevices();

        static std::optional<std::string> GetDeviceName(std::size_t id);

        static bool AddSubsystemReference();
        static void RemoveSubsystemReference();

        AudioDevice(std::size_t deviceID, std::size_t sampleRate, std::size_t channels);
        virtual ~AudioDevice() override;

        AudioDevice(const AudioDevice&) = delete;
        AudioDevice& operator=(const AudioDevice&) = delete;

        template <typename _Ty>
        bool GetAudio(StereoSignal<_Ty>& signal, std::size_t countRequested) {
            std::size_t maxSamples = countRequested * m_Channels;
            float interleaved[maxSamples];

            auto result = GetInterleavedAudio(interleaved, countRequested);
            if (!result.has_value()) {
                return false;
            }

            std::size_t count = result.value();
            if (count == 0) {
                SCHMIX_WARN("0 samples received - skipping conversion");
                return false;
            }

            if (count > countRequested) {
                SCHMIX_ERROR("Somehow received more audio than requested");
                return false;
            }

            signal = StereoSignal<_Ty>(m_Channels, count);
            for (std::size_t i = 0; i < count; i++) {
                for (std::size_t j = 0; j < m_Channels; j++) {
                    std::size_t sampleIndex = i * m_Channels + j;
                    signal[j][i] = interleaved[sampleIndex];
                }
            }

            return true;
        }

        template <typename _Ty>
        bool PutAudio(const StereoSignal<_Ty>& signal) {
            if (!signal) {
                SCHMIX_WARN("Attempted to push empty signal; skipping");
                return false;
            }

            std::size_t channels = signal.GetChannels();
            std::size_t length = signal.GetLength();

            std::size_t totalSamples = channels * length;
            float interleaved[totalSamples];

            for (std::size_t i = 0; i < totalSamples; i++) {
                std::size_t channelIndex = i % channels;
                std::size_t sampleIndex = i / channels;

                _Ty sample = signal[channelIndex][sampleIndex];
                interleaved[i] = ConvertSample<_Ty>(sample);
            }

            return PutInterleavedAudio(interleaved, totalSamples);
        }

        std::optional<std::size_t> GetInterleavedAudio(float* samples, std::size_t countRequested);
        bool PutInterleavedAudio(const float* samples, std::size_t count);

        bool Flush();

        std::size_t GetAvailableSamples() const;
        std::size_t GetQueuedSamples() const;

        std::size_t GetDeviceID() const { return m_DeviceID; }

        std::size_t GetSampleRate() const { return m_SampleRate; }
        std::size_t GetChannels() const { return m_Channels; }

        bool IsInitialized() const { return m_Initialized; }

    private:
        SDL_AudioStream* m_Stream;

        std::size_t m_DeviceID;
        std::size_t m_SampleRate, m_Channels;

        bool m_HoldsReference;
        bool m_Initialized;
    };
} // namespace schmix
