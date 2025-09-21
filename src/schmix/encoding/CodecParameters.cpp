#include "schmixpch.h"
#include "schmix/encoding/CodecParameters.h"

#include "schmix/encoding/FFmpeg.h"

namespace schmix {
    CodecParameters::CodecParameters() { m_Parameters = avcodec_parameters_alloc(); }
    CodecParameters::~CodecParameters() { Reset(); }

    CodecParameters::CodecParameters(const AVCodecParameters* source) {
        m_Parameters = avcodec_parameters_alloc();
        avcodec_parameters_copy(m_Parameters, source);
    }

    CodecParameters::CodecParameters(const CodecParameters& other) {
        m_Parameters = avcodec_parameters_alloc();
        avcodec_parameters_copy(m_Parameters, other.m_Parameters);
    }

    CodecParameters& CodecParameters::operator=(const CodecParameters& other) {
        Reset();

        m_Parameters = avcodec_parameters_alloc();
        avcodec_parameters_copy(m_Parameters, other.m_Parameters);

        return *this;
    }

    CodecParameters::CodecParameters(CodecParameters&& other) {
        m_Parameters = other.m_Parameters;
        other.m_Parameters = nullptr;
    }

    CodecParameters& CodecParameters::operator=(CodecParameters&& other) {
        Reset();

        m_Parameters = other.m_Parameters;
        other.m_Parameters = nullptr;

        return *this;
    }

    void CodecParameters::Reset() {
        if (m_Parameters == nullptr) {
            return;
        }

        avcodec_parameters_free(&m_Parameters);
    }

    std::size_t CodecParameters::GetChannels() const { return m_Parameters->ch_layout.nb_channels; }

    std::int32_t CodecParameters::GetAVSampleFormat() const {
        return (std::int32_t)m_Parameters->format;
    }
} // namespace schmix
