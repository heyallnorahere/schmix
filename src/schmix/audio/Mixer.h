#pragma once

#include "schmix/audio/Signal.h"

namespace schmix {
    class Mixer {
    public:
        using Sample = double;
        using MixerSignal = StereoSignal<Sample>;

        struct Channel {
            MixerSignal State;
            std::unordered_set<std::uint32_t> Inputs;

            double Volume;
        };

        Mixer(std::size_t chunkSize, std::size_t sampleRate, std::size_t audioChannels);
        ~Mixer();

        Mixer(const Mixer&) = delete;
        Mixer& operator=(const Mixer&) = delete;

        // will call Reset()
        void SetParameters(std::size_t chunkSize, std::size_t sampleRate,
                           std::size_t audioChannels);

        void Reset();

        void AddSignalToChannel(std::uint32_t index, const MixerSignal& data);

        MixerSignal EvaluateChannel(const Channel* channel) const;

    private:
        std::size_t m_ChunkSize, m_SampleRate, m_AudioChannels;
        std::unordered_map<std::uint32_t, Channel> m_Channels;
    };
} // namespace schmix
