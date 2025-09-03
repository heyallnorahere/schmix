namespace Schmix.Example.Waves;

public interface IWaveform
{
    public double GetPhaseCoefficient(double frequency);
    public double Calculate(double phase);

    public string Name { get; }
}
