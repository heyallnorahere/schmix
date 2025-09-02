namespace Schmix.Audio;

public interface IAudioInput
{
    public StereoSignal<double>? Signal { get; }
}
