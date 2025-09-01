namespace Schmix;

using Coral.Managed.Interop;

public class Mixer : RefCounted
{
    public Mixer(nuint address) : base(address)
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

    internal static unsafe delegate*<nuint, uint, int, int, NativeArray<double>, void> AddSignalToChannel_Impl = null;
}
