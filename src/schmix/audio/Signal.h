#pragma once

#include "schmix/core/Memory.h"

namespace schmix {
    template <typename _Sample>
    class MonoSignal {
    public:
        using Sample = _Sample;

        MonoSignal Copy(const Sample* data, std::size_t length) {
            MonoSignal dst(length);
            if (length > 0) {
                Memory::Copy(data, dst.m_Data, length * sizeof(Sample));
            }

            return dst;
        }

        MonoSignal() {
            m_Length = 0;

            m_Data = nullptr;
            m_Owned = false;
        }

        ~MonoSignal() {
            if (!m_Owned) {
                return;
            }

            Memory::Free(m_Data);
        }

        MonoSignal(std::size_t length) {
            m_Length = length;

            std::size_t size = m_Length * sizeof(Sample);
            if (size > 0) {
                m_Data = (Sample*)Memory::Allocate(size);
                Memory::Fill(m_Data, 0, size);
            } else {
                m_Data = nullptr;
            }

            m_Owned = true;
        }

        MonoSignal(const MonoSignal& src) {
            m_Length = src.m_Length;

            std::size_t size = m_Length * sizeof(Sample);
            if (size > 0) {
                m_Data = Memory::Allocate(size);
                Memory::Copy(src.m_Data, m_Data, size);
            } else {
                m_Data = nullptr;
            }

            m_Owned = true;
        }

        MonoSignal& operator=(const MonoSignal& src) {
            if (m_Owned) {
                Memory::Free(m_Data);
            }

            m_Length = src.m_Length;

            std::size_t size = m_Length * sizeof(Sample);
            if (size > 0) {
                m_Data = Memory::Allocate(size);
                Memory::Copy(src.m_Data, m_Data, size);
            } else {
                m_Data = nullptr;
            }

            m_Owned = true;

            return *this;
        }

        MonoSignal(MonoSignal&& src) {
            m_Length = src.m_Length;

            m_Data = src.m_Data;
            m_Owned = src.m_Owned;

            src.m_Owned = false;
        }

        MonoSignal& operator=(MonoSignal&& src) {
            if (m_Owned) {
                Memory::Free(m_Data);
            }

            m_Length = src.m_Length;

            m_Data = src.m_Data;
            m_Owned = src.m_Owned;

            src.m_Owned = false;

            return *this;
        }

        std::size_t GetLength() const { return m_Length; }

        Sample* GetData() { return m_Data; }
        const Sample* GetData() const { return m_Data; }

        Sample& operator[](std::size_t index) {
            if (index >= m_Length) {
                throw std::runtime_error("Index out of bounds!");
            }

            return m_Data[index];
        }

        const Sample& operator[](std::size_t index) const {
            if (index >= m_Length) {
                throw std::runtime_error("Index out of bounds!");
            }

            return m_Data[index];
        }

        bool IsPresent() const { return m_Data != nullptr && m_Length > 0; }
        bool IsEmpty() const { return m_Data == nullptr || m_Length == 0; }

        operator bool() const { return IsPresent(); }

        void Clear() {
            if (m_Owned) {
                Memory::Free(m_Data);
            }

            m_Length = 0;

            m_Data = nullptr;
            m_Owned = false;
        }

        MonoSignal operator+(const MonoSignal& other) const {
            if (m_Length != other.m_Length) {
                throw std::runtime_error("Differing signal lengths!");
            }

            MonoSignal result(m_Length);
            for (std::size_t i = 0; i < m_Length; i++) {
                result[i] = m_Data[i] + other[i];
            }

            return result;
        }

        MonoSignal& operator+=(const MonoSignal& other) {
            if (m_Length != other.m_Length) {
                throw std::runtime_error("Differing signal lengths!");
            }

            for (std::size_t i = 0; i < m_Length; i++) {
                m_Data[i] += other[i];
            }

            return *this;
        }

        MonoSignal operator-() const {
            MonoSignal result(m_Length);
            for (std::size_t i = 0; i < m_Length; i++) {
                result[i] = -m_Data[i];
            }

            return result;
        }

        MonoSignal operator-(const MonoSignal& other) const {
            if (m_Length != other.m_Length) {
                throw std::runtime_error("Differing signal lengths!");
            }

            MonoSignal result(m_Length);
            for (std::size_t i = 0; i < m_Length; i++) {
                result[i] = m_Data[i] - other[i];
            }

            return result;
        }

        MonoSignal& operator-=(const MonoSignal& other) {
            if (m_Length != other.m_Length) {
                throw std::runtime_error("Differing signal lengths!");
            }

            for (std::size_t i = 0; i < m_Length; i++) {
                m_Data[i] -= other[i];
            }

            return *this;
        }

        MonoSignal operator*(double scalar) const {
            MonoSignal result(m_Length);
            for (std::size_t i = 0; i < m_Length; i++) {
                result[i] = (Sample)(m_Data[i] * scalar);
            }

            return result;
        }

        MonoSignal& operator*=(double scalar) {
            for (std::size_t i = 0; i < m_Length; i++) {
                m_Data[i] = (Sample)(m_Data[i] * scalar);
            }

            return *this;
        }

        MonoSignal operator/(double scalar) const {
            MonoSignal result(m_Length);
            for (std::size_t i = 0; i < m_Length; i++) {
                result[i] = (Sample)(m_Data[i] / scalar);
            }

            return result;
        }

        MonoSignal& operator/=(double scalar) {
            for (std::size_t i = 0; i < m_Length; i++) {
                m_Data[i] = (Sample)(m_Data[i] / scalar);
            }

            return *this;
        }

    private:
        std::size_t m_Length;

        Sample* m_Data;
        bool m_Owned;
    };

    template <typename _Sample>
    class StereoSignal {
    public:
        using Sample = _Sample;
        using Component = MonoSignal<Sample>;

        StereoSignal() {
            m_Channels = 0;
            m_Length = 0;

            m_Data = nullptr;
            m_Owned = false;
        }

        ~StereoSignal() {
            if (m_Owned) {
                delete[] m_Data;
            }
        }

        StereoSignal(std::size_t channels, std::size_t length) {
            m_Channels = channels;
            m_Length = length;

            if (m_Channels > 0) {
                m_Data = new Component[m_Channels];
                for (size_t i = 0; i < m_Channels; i++) {
                    m_Data[i] = Component(m_Length);
                }
            } else {
                m_Data = nullptr;
            }

            m_Owned = true;
        }

        StereoSignal(const StereoSignal& other) {
            m_Channels = other.m_Channels;
            m_Length = other.m_Length;

            if (m_Channels > 0) {
                m_Data = new Component[m_Channels];
                for (size_t i = 0; i < m_Channels; i++) {
                    // copy
                    m_Data[i] = other.m_Data[i];
                }
            } else {
                m_Data = nullptr;
            }

            m_Owned = true;
        }

        StereoSignal& operator=(const StereoSignal& other) {
            if (m_Owned) {
                delete[] m_Data;
            }

            m_Channels = other.m_Channels;
            m_Length = other.m_Length;

            if (m_Channels > 0) {
                m_Data = new Component[m_Channels];
                for (size_t i = 0; i < m_Channels; i++) {
                    // copy
                    m_Data[i] = other.m_Data[i];
                }
            } else {
                m_Data = nullptr;
            }

            m_Owned = true;

            return *this;
        }

        StereoSignal(StereoSignal&& other) {
            m_Channels = other.m_Channels;
            m_Length = other.m_Length;

            m_Data = other.m_Data;
            m_Owned = other.m_Owned;

            other.m_Owned = false;
        }

        StereoSignal& operator=(StereoSignal&& other) {
            if (m_Owned) {
                delete[] m_Data;
            }

            m_Channels = other.m_Channels;
            m_Length = other.m_Length;

            m_Data = other.m_Data;
            m_Owned = other.m_Owned;

            other.m_Owned = false;

            return *this;
        }

        std::size_t GetChannels() const { return m_Channels; }
        std::size_t GetLength() const { return m_Length; }

        Component& operator[](std::size_t index) {
            if (index >= m_Channels) {
                throw std::runtime_error("Index out of range!");
            }

            return m_Data[index];
        }

        const Component& operator[](std::size_t index) const {
            if (index >= m_Channels) {
                throw std::runtime_error("Index out of range!");
            }

            return m_Data[index];
        }

        bool IsPresent() const { return m_Channels > 0 && m_Length > 0 && m_Data != nullptr; }
        bool IsEmpty() const { return m_Channels == 0 || m_Length == 0 || m_Data == nullptr; }

        operator bool() const { return IsPresent(); }

        void Clear() {
            if (m_Owned) {
                delete[] m_Data;
            }

            m_Channels = 0;
            m_Length = 0;

            m_Data = nullptr;
            m_Owned = false;
        }

        StereoSignal operator+(const StereoSignal& other) const {
            if (m_Channels != other.m_Channels) {
                throw std::runtime_error("Differing channel counts!");
            }

            StereoSignal result(m_Channels, m_Length);
            for (std::size_t i = 0; i < m_Channels; i++) {
                result[i] = m_Data[i] + other[i];
            }
        }

        StereoSignal& operator+=(const StereoSignal& other) {
            if (m_Channels != other.m_Channels) {
                throw std::runtime_error("Differing channel counts!");
            }

            for (std::size_t i = 0; i < m_Channels; i++) {
                m_Data[i] += other[i];
            }
        }

        StereoSignal operator-() const {
            StereoSignal result(m_Channels, m_Length);
            for (std::size_t i = 0; i < m_Channels; i++) {
                result[i] = -m_Data[i];
            }
        }

        StereoSignal operator-(const StereoSignal& other) const {
            if (m_Channels != other.m_Channels) {
                throw std::runtime_error("Differing channel counts!");
            }

            StereoSignal result(m_Channels, m_Length);
            for (std::size_t i = 0; i < m_Channels; i++) {
                result[i] = m_Data[i] - other[i];
            }
        }

        StereoSignal& operator-=(const StereoSignal& other) {
            if (m_Channels != other.m_Channels) {
                throw std::runtime_error("Differing channel counts!");
            }

            for (std::size_t i = 0; i < m_Channels; i++) {
                m_Data[i] -= other[i];
            }
        }

        StereoSignal operator*(double scalar) const {
            StereoSignal result(m_Channels, m_Length);
            for (std::size_t i = 0; i < m_Channels; i++) {
                result[i] = m_Data[i] * scalar;
            }
        }

        StereoSignal& operator*=(double scalar) {
            for (std::size_t i = 0; i < m_Channels; i++) {
                m_Data[i] *= scalar;
            }
        }

        StereoSignal operator/(double scalar) const {
            StereoSignal result(m_Channels, m_Length);
            for (std::size_t i = 0; i < m_Channels; i++) {
                result[i] = m_Data[i] / scalar;
            }
        }

        StereoSignal& operator/=(double scalar) {
            for (std::size_t i = 0; i < m_Channels; i++) {
                m_Data[i] /= scalar;
            }
        }

    private:
        std::size_t m_Channels, m_Length;

        Component* m_Data;
        bool m_Owned;
    };
} // namespace schmix
