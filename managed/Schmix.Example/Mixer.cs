namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Extension;

using System;
using System.Collections.Generic;

internal sealed class MixerModule : Module
{
    public const int MixerInputCount = 10;

    public MixerModule()
    {
        mGains = new double[MixerInputCount];
        Array.Fill(mGains, 1);
    }

    public override int InputCount => mGains.Length;
    public override int OutputCount => 1;

    public override string GetInputName(int index) => $"Input {index + 1}";
    public override string GetOutputName(int index) => index > 0 ? "<unused>" : "Output";

    public override string Name => "Mixer";

    public override void DrawProperties()
    {
        ImGui.PushItemWidth(100f);

        for (int i = 0; i < mGains.Length; i++)
        {
            float gain = (float)mGains[i];
            if (ImGui.SliderFloat($"Gain {i + 1}", ref gain, 0f, 1f))
            {
                mGains[i] = (double)gain;
            }
        }

        ImGui.PopItemWidth();
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        var output = outputs[0];
        if (output is null)
        {
            return;
        }

        var outputSignal = new StereoSignal<double>(channels, samplesRequested);
        for (int i = 0; i < mGains.Length; i++)
        {
            var input = inputs[i];
            var inputSignal = input?.Signal;

            if (inputSignal is null)
            {
                continue;
            }

            double gain = mGains[i];
            outputSignal += inputSignal * gain;
        }

        output.PutSignal(outputSignal);
    }

    private readonly double[] mGains;
}

[RegisteredPlugin("Mixer")]
public sealed class MixerPlugin : Plugin
{
    public override Module Instantiate() => new MixerModule();
}
