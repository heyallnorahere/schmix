namespace Schmix.Audio;

using Coral.Managed.Interop;

using Schmix.Core;

using System;

public sealed class OutputDevice : RefCounted
{
    public static unsafe uint Default => GetDefault_Impl();

    internal static unsafe void* CreateOutput(uint deviceID, int sampleRate, int channels)
    {
        Log.Debug("Creating managed audio output...");

        void* address = ctor_Impl(deviceID, sampleRate, channels);
        if (address is null)
        {
            throw new SystemException($"Failed to initialize audio output with ID: {deviceID}");
        }

        return address;
    }

    public unsafe OutputDevice(uint deviceID, int sampleRate, int channels) : base(CreateOutput(deviceID, sampleRate, channels))
    {
    }

    public bool PutAudio(StereoSignal<double> signal)
    {
        double[] interleaved = signal.AsInterleaved();
        using var nativeInterleaved = new NativeArray<double>(interleaved);

        bool success;
        unsafe
        {
            success = PutAudio_Impl(mAddress, signal.Length, nativeInterleaved);
        }

        return success;
    }

    public unsafe int QueuedSamples => GetQueuedSamples_Impl(mAddress);

    public unsafe int SampleRate => GetSampleRate_Impl(mAddress);
    public unsafe int Channels => GetChannels_Impl(mAddress);

    internal static unsafe delegate*<uint> GetDefault_Impl = null;
    internal static unsafe delegate*<uint, int, int, void*> ctor_Impl = null;

    internal static unsafe delegate*<void*, int> GetQueuedSamples_Impl = null;

    internal static unsafe delegate*<void*, int> GetSampleRate_Impl = null;
    internal static unsafe delegate*<void*, int> GetChannels_Impl = null;

    internal static unsafe delegate*<void*, int, NativeArray<double>, Bool32> PutAudio_Impl = null;
}
