namespace Schmix.Audio;

using System;
using System.Numerics;

public sealed class StereoSignal<T> where T : unmanaged, INumber<T>
{
    public StereoSignal(int channels, int length)
    {
        mLength = length;

        mChannels = new MonoSignal<T>[channels];
        for (int i = 0; i < channels; i++)
        {
            mChannels[i] = new MonoSignal<T>(length);
        }
    }

    public StereoSignal(int channels, ReadOnlySpan<T> interleavedData)
    {
        int totalSamples = interleavedData.Length;
        mLength = totalSamples / channels;

        var sequentialData = new T[mLength * channels];
        for (int i = 0; i < channels; i++)
        {
            for (int j = 0; j < mLength; j++)
            {
                int interleavedIndex = j * channels + i;
                int sequentialIndex = i * mLength + j;

                sequentialData[sequentialIndex] = interleavedData[interleavedIndex];
            }
        }

        mChannels = new MonoSignal<T>[channels];
        for (int i = 0; i < channels; i++)
        {
            var slice = sequentialData.AsSpan().Slice(mLength * i, mLength);
            mChannels[i] = new MonoSignal<T>(slice);
        }
    }

    private StereoSignal()
    {
        mLength = 0;
        mChannels = Array.Empty<MonoSignal<T>>();
    }

    public int Channels => mChannels.Length;
    public int Length => mLength;

    public MonoSignal<T> this[int channel] => mChannels[channel];

    public T[] AsInterleaved()
    {
        var interleaved = new T[mLength * mChannels.Length];

        for (int i = 0; i < mLength; i++)
        {
            int offset = i * mChannels.Length;
            for (int j = 0; j < mChannels.Length; j++)
            {
                var mono = mChannels[j];
                var interleavedIndex = offset + j;

                interleaved[interleavedIndex] = mono[i];
            }
        }

        return interleaved;
    }

    public StereoSignal<T> Copy()
    {
        var result = new StereoSignal<T>();
        result.mLength = mLength;

        result.mChannels = new MonoSignal<T>[mChannels.Length];
        for (int i = 0; i < mChannels.Length; i++)
        {
            result.mChannels[i] = new MonoSignal<T>(mChannels[i].Data);
        }

        return result;
    }

    public static StereoSignal<T> operator +(StereoSignal<T> lhs, StereoSignal<T> rhs)
    {
        if (lhs.mChannels.Length != rhs.mChannels.Length)
        {
            throw new ArgumentException("Differing channel count!");
        }

        int channels = lhs.mChannels.Length;
        var result = new StereoSignal<T>(channels, lhs.mLength);

        for (int i = 0; i < channels; i++)
        {
            result.mChannels[i] = lhs[i] + rhs[i];
        }

        return result;
    }

    public static StereoSignal<T> operator -(StereoSignal<T> signal)
    {
        int channels = signal.mChannels.Length;
        var result = new StereoSignal<T>(channels, signal.mLength);

        for (int i = 0; i < channels; i++)
        {
            result.mChannels[i] = -signal[i];
        }

        return result;
    }

    public static StereoSignal<T> operator -(StereoSignal<T> lhs, StereoSignal<T> rhs)
    {
        if (lhs.mChannels.Length != rhs.mChannels.Length)
        {
            throw new ArgumentException("Differing channel count!");
        }

        int channels = lhs.mChannels.Length;
        var result = new StereoSignal<T>(channels, lhs.mLength);

        for (int i = 0; i < channels; i++)
        {
            result.mChannels[i] = lhs[i] - rhs[i];
        }

        return result;
    }

    public static StereoSignal<T> operator *(StereoSignal<T> signal, double scalar)
    {
        int channels = signal.mChannels.Length;
        var result = new StereoSignal<T>(channels, signal.mLength);

        for (int i = 0; i < channels; i++)
        {
            result.mChannels[i] = signal[i] * scalar;
        }

        return result;
    }

    public static StereoSignal<T> operator /(StereoSignal<T> signal, double scalar)
    {
        int channels = signal.mChannels.Length;
        var result = new StereoSignal<T>(channels, signal.mLength);

        for (int i = 0; i < channels; i++)
        {
            result.mChannels[i] = signal[i] / scalar;
        }

        return result;
    }

    public StereoSignal<T> Exp(double expBase)
    {
        int channels = mChannels.Length;
        var result = new StereoSignal<T>(channels, mLength);

        for (int i = 0; i < channels; i++)
        {
            result.mChannels[i] = mChannels[i].Exp(expBase);
        }

        return result;
    }

    private int mLength;
    private MonoSignal<T>[] mChannels;
}
