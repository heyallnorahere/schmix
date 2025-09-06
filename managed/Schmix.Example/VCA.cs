namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Extension;

using System.Collections.Generic;

internal sealed class VCAModule : Module
{
    public VCAModule()
    {
        mBaseGain = 1;
    }

    private const int CVInput = 0;
    private const int SignalInput = 1;

    public override int InputCount => 2;
    public override int OutputCount => 1;

    public override string GetInputName(int index) => index switch
    {
        CVInput => "CV",
        SignalInput => "Signal",
        _ => "<unused>"
    };

    public override string GetOutputName(int index) => index > 0 ? "<unused>" : "Output";

    public override string Name => "VCA";

    public override void DrawProperties()
    {
        ImGui.PushItemWidth(100f);

        float baseGain = (float)mBaseGain;
        if (ImGui.SliderFloat("Base gain", ref baseGain, -1f, 1f))
        {
            mBaseGain = baseGain;
        }

        ImGui.PopItemWidth();
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        var cvInput = inputs[CVInput];
        var signalInput = inputs[SignalInput];

        var cvSignal = cvInput?.Signal;
        var gainSignal = cvSignal?.Exp(2); // cv is volt per amplitude

        var signal = signalInput?.Signal;
        var output = outputs[0];

        if (signal is null || output is null)
        {
            return;
        }

        var result = new StereoSignal<double>(channels, samplesRequested);
        for (int i = 0; i < channels; i++)
        {
            for (int j = 0; j < samplesRequested; j++)
            {
                double gain = mBaseGain;
                if (gainSignal is not null)
                {
                    gain *= gainSignal[i][j];
                }

                double srcSample = signal[i][j];
                double dstSample = srcSample * gain;

                result[i][j] = dstSample;
            }
        }

        output.PutSignal(result);
    }

    private double mBaseGain;
}

[RegisteredPlugin("VCA")]
public sealed class VCAPlugin : Plugin
{
    public override Module Instantiate() => new VCAModule();
}
