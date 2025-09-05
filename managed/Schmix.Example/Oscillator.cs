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
        const float width = 100f;

        float baseFrequency = (float)mBaseFrequency;
        ImGui.SetNextItemWidth(width);

        if (ImGui.DragFloat("Base frequency", ref baseFrequency))
        {
            mBaseFrequency = baseFrequency;
        }
    }

    public const int GateInput = 0;
    public const int CVInput = 1;

    public const int SineOutput = 0;

    public override string GetInputName(int index) => index switch
    {
        GateInput => "Gate",
        CVInput => "CV",
        _ => "<unused>"
    };

    public override string GetOutputName(int index)
    {
        if (index >= sWaves.Count)
        {
            return "<unused>";
        }

        return sWaves[index].Name;
    }

    public override string Name => "Oscillator";

    public override int InputCount => 2;
    public override int OutputCount => sWaves.Count;

    private static StereoSignal<double> CVToPower(StereoSignal<double> signal, double powerBase)
    {
        var result = new StereoSignal<double>(signal.Channels, signal.Length);
        for (int i = 0; i < result.Channels; i++)
        {
            var srcChannel = signal[i];
            var dstChannel = result[i];

            for (int j = 0; j < result.Length; j++)
            {
                double srcSample = srcChannel[j];
                double dstSample = Math.Pow(powerBase, srcSample);

                dstChannel[j] = dstSample;
            }
        }

        return result;
    }

    public OscillatorModule()
    {
        mBaseFrequency = 440;

        mPhases = new double[sWaves.Count][];
        Array.Fill(mPhases, Array.Empty<double>());
    }

    public override void Process(IReadOnlyList<IAudioInput?> inputs, IReadOnlyList<IAudioOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        IAudioInput? gateInput = inputs[GateInput];
        IAudioInput? cvInput = inputs[CVInput];

        // generate signal of coefficients over time to modify the frequency
        StereoSignal<double>? frequencyCoefficients = null;
        var cvSignal = cvInput?.Signal;

        if (cvSignal is not null)
        {
            // using cv as volt per octave
            // octave ratio is 2:1
            frequencyCoefficients = CVToPower(cvSignal, 2);
        }

        var gateSignal = gateInput?.Signal;
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
                    double gate = 1;
                    if (gateSignal is not null)
                    {
                        gate = gateSignal[j][k];
                    }

                    double sample = 0;
                    if (gate > 0.1)
                    {
                        double sampleFrequency = mBaseFrequency;
                        if (frequencyCoefficients is not null)
                        {
                            sampleFrequency *= frequencyCoefficients[j][k];
                        }

                        var waveform = sWaves[i];
                        double phaseCoefficient = waveform.GetPhaseCoefficient(sampleFrequency);

                        sample = waveform.Calculate(phase);
                        phase += phaseCoefficient / (double)sampleRate;
                    }

                    signal[j][k] = sample;
                }

                wavePhases[j] = phase;
            }

            currentOutput.PutAudio(signal);
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
