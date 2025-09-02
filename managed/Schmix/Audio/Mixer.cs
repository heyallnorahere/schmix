namespace Schmix.Audio;

using Coral.Managed.Interop;

using Schmix.Core;

public class Mixer : RefCounted
{
    internal unsafe Mixer(void* address) : base(address)
    {
    }

    public void AddSignalToChannel(uint channel, StereoSignal<double> signal)
    {
        double[] interleaved = signal.AsInterleaved();
        var nativeData = new NativeArray<double>(interleaved);

        unsafe
        {
            AddSignalToChannel_Impl(mAddress, channel, signal.Channels, signal.Length, nativeData);
        }
    }

    public int ChunkSize
    {
        get
        {
            unsafe
            {
                return GetChunkSize_Impl(mAddress);
            }
        }
    }

    public int SampleRate
    {
        get
        {
            unsafe
            {
                return GetSampleRate_Impl(mAddress);
            }
        }
    }

    public int AudioChannels
    {
        get
        {
            unsafe
            {
                return GetAudioChannels_Impl(mAddress);
            }
        }
    }

    internal static unsafe delegate*<void*, uint, int, int, NativeArray<double>, void> AddSignalToChannel_Impl = null;
    internal static unsafe delegate*<void*, int> GetChunkSize_Impl = null;
    internal static unsafe delegate*<void*, int> GetSampleRate_Impl = null;
    internal static unsafe delegate*<void*, int> GetAudioChannels_Impl = null;
}
