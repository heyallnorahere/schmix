namespace Schmix.Audio;

public interface ISignalInput
{
    public StereoSignal<double>? Signal { get; }
}

public interface ISignalOutput
{
    public void PutSignal(StereoSignal<double> signal);
}
