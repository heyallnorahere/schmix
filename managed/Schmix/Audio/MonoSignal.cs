namespace Schmix.Audio;

using System;
using System.Numerics;

public sealed class MonoSignal<T> where T : unmanaged, INumber<T>
{
    public MonoSignal(int length)
    {
        mData = new T[length];
        Array.Clear(mData);
    }

    public MonoSignal(ReadOnlySpan<T> data)
    {
        mData = new T[data.Length];
        data.CopyTo(mData);
    }

    public T this[int index]
    {
        get => mData[index];
        set => mData[index] = value;
    }

    public int Length => mData.Length;

    internal T[] Data => mData;

    public static MonoSignal<T> operator +(MonoSignal<T> lhs, MonoSignal<T> rhs)
    {
        if (lhs.Length != rhs.Length)
        {
            throw new ArgumentException("Signal length mismatch!");
        }

        int length = lhs.Length;
        var result = new MonoSignal<T>(length);

        for (int i = 0; i < length; i++)
        {
            result[i] = lhs[i] + rhs[i];
        }

        return result;
    }

    public static MonoSignal<T> operator -(MonoSignal<T> signal)
    {
        int length = signal.Length;
        var result = new MonoSignal<T>(length);

        for (int i = 0; i < length; i++)
        {
            result[i] = -signal[i];
        }

        return result;
    }

    public static MonoSignal<T> operator -(MonoSignal<T> lhs, MonoSignal<T> rhs)
    {
        if (lhs.Length != rhs.Length)
        {
            throw new ArgumentException("Signal length mismatch!");
        }

        int length = lhs.Length;
        var result = new MonoSignal<T>(length);

        for (int i = 0; i < length; i++)
        {
            result[i] = lhs[i] - rhs[i];
        }

        return result;
    }

    public static MonoSignal<T> operator *(MonoSignal<T> signal, double scalar)
    {
        int length = signal.Length;
        var result = new MonoSignal<T>(length);

        for (int i = 0; i < length; i++)
        {
            double value = Convert.ToDouble(signal[i]) * scalar;
            result[i] = (T)Convert.ChangeType(value, typeof(T));
        }

        return result;
    }

    public static MonoSignal<T> operator *(double scalar, MonoSignal<T> signal)
    {
        return signal * scalar;
    }

    public static MonoSignal<T> operator /(MonoSignal<T> signal, double scalar)
    {
        int length = signal.Length;
        var result = new MonoSignal<T>(length);

        for (int i = 0; i < length; i++)
        {
            double value = Convert.ToDouble(signal[i]) / scalar;
            result[i] = (T)Convert.ChangeType(value, typeof(T));
        }

        return result;
    }

    private T[] mData;
}
