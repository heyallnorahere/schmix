namespace Schmix.Audio;

public interface IAudioOutput
{
    public void ResetSignal();

    public void PutAudio(StereoSignal<double> signal);
}
