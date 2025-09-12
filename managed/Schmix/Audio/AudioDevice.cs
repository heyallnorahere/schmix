namespace Schmix.Audio;

using Coral.Managed.Interop;

using Schmix.Core;

using System;
using System.Collections.Generic;

public sealed class AudioDevice : RefCounted
{
    public static unsafe uint DefaultInput => GetDefaultInput_Impl();
    public static unsafe uint DefaultOutput => GetDefaultOutput_Impl();

    public static unsafe string GetDeviceName(uint id) => GetDeviceName_Impl(id).ToString() ?? $"<device {id}>";

    internal static IReadOnlyDictionary<uint, string> CollectDeviceNames(IReadOnlyList<uint> ids)
    {
        var result = new Dictionary<uint, string>();
        for (int i = 0; i < ids.Count; i++)
        {
            uint id = ids[i];
            var name = GetDeviceName(id);

            result.Add(id, name);
        }

        return result;
    }

    public static unsafe IReadOnlyDictionary<uint, string> InputDevices
    {
        get
        {
            using var ids = GetInputDevices_Impl();
            return CollectDeviceNames(ids.ToArray());
        }
    }

    public static unsafe IReadOnlyDictionary<uint, string> OutputDevices
    {
        get
        {
            using var ids = GetOutputDevices_Impl();
            return CollectDeviceNames(ids.ToArray());
        }
    }

    internal static unsafe void* Open(uint deviceID, int sampleRate, int channels)
    {
        Log.Debug("Opening managed audio...");

        void* address = ctor_Impl(deviceID, sampleRate, channels);
        if (address is null)
        {
            throw new SystemException($"Failed to initialize audio device with ID: {deviceID}");
        }

        return address;
    }

    public unsafe AudioDevice(uint deviceID, int sampleRate, int channels) : base(Open(deviceID, sampleRate, channels))
    {
    }

    public StereoSignal<double>? GetAudio(int samplesRequested)
    {
        int channels = Channels;
        using var interleaved = new NativeArray<double>(samplesRequested * channels);

        int samplesReceived;
        unsafe
        {
            samplesReceived = GetAudio_Impl(mAddress, samplesRequested, interleaved);
        }

        if (samplesReceived < 0)
        {
            return null;
        }

        var data = interleaved.ToSpan().Slice(0, samplesReceived * channels);
        return new StereoSignal<double>(channels, data);
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

    public unsafe bool Flush() => Flush_Impl(mAddress);

    public unsafe uint DeviceID => GetDeviceID_Impl(mAddress);

    public unsafe int AvailableSamples => GetAvailableSamples_Impl(mAddress);
    public unsafe int QueuedSamples => GetQueuedSamples_Impl(mAddress);

    public unsafe int SampleRate => GetSampleRate_Impl(mAddress);
    public unsafe int Channels => GetChannels_Impl(mAddress);

    internal static unsafe delegate*<uint> GetDefaultInput_Impl = null;
    internal static unsafe delegate*<uint> GetDefaultOutput_Impl = null;

    internal static unsafe delegate*<NativeArray<uint>> GetInputDevices_Impl = null;
    internal static unsafe delegate*<NativeArray<uint>> GetOutputDevices_Impl = null;

    internal static unsafe delegate*<uint, NativeString> GetDeviceName_Impl = null;

    internal static unsafe delegate*<uint, int, int, void*> ctor_Impl = null;

    internal static unsafe delegate*<void*, uint> GetDeviceID_Impl = null;

    internal static unsafe delegate*<void*, int> GetAvailableSamples_Impl = null;
    internal static unsafe delegate*<void*, int> GetQueuedSamples_Impl = null;

    internal static unsafe delegate*<void*, int> GetSampleRate_Impl = null;
    internal static unsafe delegate*<void*, int> GetChannels_Impl = null;

    internal static unsafe delegate*<void*, int, NativeArray<double>, int> GetAudio_Impl = null;
    internal static unsafe delegate*<void*, int, NativeArray<double>, Bool32> PutAudio_Impl = null;

    internal static unsafe delegate*<void*, Bool32> Flush_Impl = null;
}
