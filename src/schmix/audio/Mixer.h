#pragma once

#include "schmix/audio/Signal.h"

namespace schmix {
    class Mixer {
    public:
        using Signal = StereoSignal<double>;

        struct Channel {
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

        std::size_t GetChunkSize() const { return m_ChunkSize; }
        std::size_t GetSampleRate() const { return m_SampleRate; }
        std::size_t GetAudioChannels() const { return m_AudioChannels; }

        Channel* GetChannel(std::uint32_t index);
        const Channel* GetChannel(std::uint32_t index) const;

        // resets all audio signals
        void Reset();

        void AddSignalToChannel(std::uint32_t index, const Signal& data);
        Signal EvaluateChannel(std::uint32_t index) const;

    private:
        void EvaluateChannelToCache(std::uint32_t index,
                                    std::unordered_map<std::uint32_t, Signal>& cache,
                                    std::unordered_set<std::uint32_t>& evaluatingChannels) const;

        std::size_t m_ChunkSize, m_SampleRate, m_AudioChannels;

        std::unordered_map<std::uint32_t, Channel> m_Channels;
        std::unordered_map<std::uint32_t, Signal> m_ChannelSignals;
    };
} // namespace schmix
