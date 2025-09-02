namespace Schmix.Audio;

using Coral.Managed.Interop;

using Schmix.Core;

using System;

public sealed class WindowAudioOutput : RefCounted, IAudioOutput
{
    public static unsafe uint DefaultDeviceID => GetDefaultDeviceID_Impl();

    internal static unsafe void* CreateOutput(uint deviceID, int sampleRate, int channels)
    {
        void* address = ctor_Impl(deviceID, sampleRate, channels);
        if (address is null)
        {
            throw new SystemException($"Failed to initialize audio output with ID: {deviceID}");
        }

        return address;
    }

    public unsafe WindowAudioOutput(uint deviceID, int sampleRate, int channels) : base(CreateOutput(deviceID, sampleRate, channels))
    {
        ResetSignal();
    }

    public void ResetSignal()
    {
        mSignal = null;
    }

    public void PutAudio(StereoSignal<double> signal)
    {
        if (signal.Channels != Channels)
        {
            throw new ArgumentException("Invalid signal channel count!");
        }

        if (mSignal is null)
        {
            mSignal = signal.Copy();
        }
        else
        {
            mSignal += signal;
        }
    }

    public bool Flush()
    {
        if (mSignal is null)
        {
            return true;
        }

        double[] interleaved = mSignal.AsInterleaved();
        using var nativeInterleaved = new NativeArray<double>(interleaved);

        bool success;
        unsafe
        {
            success = PutAudio_Impl(mAddress, mSignal.Length, nativeInterleaved);
        }

        ResetSignal();
        return success;
    }

    public unsafe int QueuedSamples => GetQueuedSamples_Impl(mAddress);

    public unsafe int SampleRate => GetSampleRate_Impl(mAddress);
    public unsafe int Channels => GetChannels_Impl(mAddress);

    private StereoSignal<double>? mSignal;

    internal static unsafe delegate*<uint> GetDefaultDeviceID_Impl = null;
    internal static unsafe delegate*<uint, int, int, void*> ctor_Impl = null;

    internal static unsafe delegate*<void*, int> GetQueuedSamples_Impl = null;

    internal static unsafe delegate*<void*, int> GetSampleRate_Impl = null;
    internal static unsafe delegate*<void*, int> GetChannels_Impl = null;

    internal static unsafe delegate*<void*, int, NativeArray<double>, Bool32> PutAudio_Impl = null;
}
