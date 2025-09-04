namespace Schmix.Audio;

public interface IAudioOutput
{
    public void PutAudio(StereoSignal<double> signal);
}
