namespace Schmix.Example.Waves;

using System;

public sealed class SineWave : IWaveform
{
    public double GetPhaseCoefficient(double frequency) => 2 * Math.PI * frequency;
    public double Calculate(double phase) => Math.Sin(phase);

    public string Name => "Sine";
}
