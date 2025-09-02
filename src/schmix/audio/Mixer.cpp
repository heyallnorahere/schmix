#include "schmixpch.h"
#include "schmix/audio/Mixer.h"

namespace schmix {
    Mixer::Mixer(std::size_t chunkSize, std::size_t sampleRate, std::size_t audioChannels) {
        m_ChunkSize = chunkSize;
        m_SampleRate = sampleRate;
        m_AudioChannels = audioChannels;
    }

    void Mixer::SetParameters(std::size_t chunkSize, std::size_t sampleRate,
                              std::size_t audioChannels) {
        Reset();

        m_ChunkSize = chunkSize;
        m_SampleRate = sampleRate;
        m_AudioChannels = audioChannels;
    }

    Mixer::Channel* Mixer::GetChannel(std::uint32_t index) {
        bool channelExists = m_Channels.contains(index);

        Mixer::Channel* channel = &m_Channels[index];
        if (!channelExists) {
            // defaults
            channel->Volume = 1;
        }

        return channel;
    }

    const Mixer::Channel* Mixer::GetChannel(std::uint32_t index) const {
        if (!m_Channels.contains(index)) {
            return nullptr;
        }

        return &m_Channels.at(index);
    }

    void Mixer::Reset() { m_ChannelSignals.clear(); }

    void Mixer::AddSignalToChannel(std::uint32_t index, const Signal& data) {
        if (data.GetChannels() != m_AudioChannels) {
            throw std::runtime_error("Invalid number of audio channels!");
        }

        if (data.GetLength() != m_ChunkSize) {
            throw std::runtime_error("Invalid chunk size!");
        }

        if (!m_ChannelSignals.contains(index)) {
            m_ChannelSignals[index] = data;
        } else {
            m_ChannelSignals[index] += data;
        }
    }

    Mixer::Signal Mixer::EvaluateChannel(std::uint32_t index) const {
        std::unordered_map<std::uint32_t, Signal> cache;
        std::unordered_set<std::uint32_t> evaluatingChannels;

        EvaluateChannelToCache(index, cache, evaluatingChannels);

        Signal result;
        if (cache.contains(index)) {
            result = cache.at(index);
        }

        return result;
    }

    void Mixer::EvaluateChannelToCache(
        std::uint32_t index, std::unordered_map<std::uint32_t, Signal>& cache,
        std::unordered_set<std::uint32_t>& evaluatingChannels) const {
        if (cache.contains(index) || evaluatingChannels.contains(index)) {
            return;
        }

        evaluatingChannels.insert(index);

        Signal result;
        if (m_ChannelSignals.contains(index)) {
            result += m_ChannelSignals.at(index);
        }

        double volume = 1;
        if (m_Channels.contains(index)) {
            const auto& channel = m_Channels.at(index);
            volume = channel.Volume;

            for (const auto& inputIndex : channel.Inputs) {
                EvaluateChannelToCache(inputIndex, cache, evaluatingChannels);

                if (cache.contains(inputIndex)) {
                    result += cache.at(inputIndex);
                }
            }
        }

        // todo: evaluate channel plugins

        cache[index] = result * volume;
        evaluatingChannels.erase(index);
    }
} // namespace schmix
