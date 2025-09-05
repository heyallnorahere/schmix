#pragma once
#include "schmix/core/Ref.h"

#include "schmix/audio/Signal.h"

typedef struct SDL_AudioStream SDL_AudioStream;

namespace schmix {
    class OutputDevice : public RefCounted {
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

        static std::size_t GetDefaultDeviceID();

        static bool AddSubsystemReference();
        static void RemoveSubsystemReference();

        OutputDevice(std::size_t deviceID, std::size_t sampleRate, std::size_t channels);
        virtual ~OutputDevice() override;

        OutputDevice(const OutputDevice&) = delete;
        OutputDevice& operator=(const OutputDevice&) = delete;

        template <typename _Ty>
        bool PutAudio(const StereoSignal<_Ty>& signal) {
            if (!signal) {
                SCHMIX_WARN("Attempted to push empty signal; skipping");
                return false;
            }

            std::size_t channels = signal.GetChannels();
            std::size_t length = signal.GetLength();

            MonoSignal<float> interleaved(length * channels);

            std::size_t totalSamples = interleaved.GetLength();
            for (std::size_t i = 0; i < totalSamples; i++) {
                std::size_t channelIndex = i % channels;
                std::size_t sampleIndex = i / channels;

                _Ty sample = signal[channelIndex][sampleIndex];
                interleaved[i] = ConvertSample<_Ty>(sample);
            }

            return PutInterleavedAudio(interleaved);
        }

        bool PutInterleavedAudio(const MonoSignal<float>& interleaved);

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
