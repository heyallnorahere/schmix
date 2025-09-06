namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Example.Waves;
using Schmix.Extension;

using System;
using System.Collections.Generic;

internal sealed class OscillatorModule : Module
{
    private static readonly IReadOnlyList<IWaveform> sWaves = new IWaveform[]
    {
        new SineWave()
    };

    public override void DrawProperties()
    {
        ImGui.PushItemWidth(100f);

        float baseFrequency = (float)mBaseFrequency;
        if (ImGui.DragFloat("Base frequency", ref baseFrequency))
        {
            mBaseFrequency = baseFrequency;
        }

        ImGui.PopItemWidth();
    }

    public override string GetInputName(int index) => index > 0 ? "<unused>" : "CV";

    public override string GetOutputName(int index)
    {
        if (index >= sWaves.Count)
        {
            return "<unused>";
        }

        return sWaves[index].Name;
    }

    public override string Name => "Oscillator";

    public override int InputCount => 1;
    public override int OutputCount => sWaves.Count;

    public OscillatorModule()
    {
        mBaseFrequency = 261.63; // C4; middle C

        mPhases = new double[sWaves.Count][];
        Array.Fill(mPhases, Array.Empty<double>());
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        var cvInput = inputs[0];

        // generate signal of coefficients over time to modify the frequency
        StereoSignal<double>? frequencyCoefficients = null;
        var cvSignal = cvInput?.Signal;

        if (cvSignal is not null)
        {
            // using cv as volt per octave
            // octave ratio is 2:1
            frequencyCoefficients = cvSignal.Exp(2);
        }

        for (int i = 0; i < outputs.Count; i++)
        {
            var currentOutput = outputs[i];
            if (currentOutput is null)
            {
                continue;
            }

            var wavePhases = mPhases[i];
            if (wavePhases.Length < channels)
            {
                wavePhases = new double[channels];
                Array.Fill(wavePhases, 0);

                mPhases[i] = wavePhases;
            }

            var signal = new StereoSignal<double>(channels, samplesRequested);
            for (int j = 0; j < channels; j++)
            {
                double phase = wavePhases[j];

                for (int k = 0; k < samplesRequested; k++)
                {
                    double sampleFrequency = mBaseFrequency;
                    if (frequencyCoefficients is not null)
                    {
                        sampleFrequency *= frequencyCoefficients[j][k];
                    }

                    var waveform = sWaves[i];
                    double phaseCoefficient = waveform.GetPhaseCoefficient(sampleFrequency);

                    double sample = waveform.Calculate(phase);
                    phase += phaseCoefficient / (double)sampleRate;

                    signal[j][k] = sample;
                }

                wavePhases[j] = phase;
            }

            currentOutput.PutSignal(signal);
        }
    }

    private double mBaseFrequency;
    private readonly double[][] mPhases;
}

[RegisteredPlugin("Oscillator")]
public sealed class OscillatorPlugin : Plugin
{
    public override Module Instantiate() => new OscillatorModule();
}
