namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Extension;

using System.Collections.Generic;

internal sealed class ConstantOutputModule : Module
{
    private double mValue;
    public ConstantOutputModule()
    {
        mValue = 0;
    }

    public override string Name => "Constant output";
    public override int OutputCount => 1;

    public override string GetOutputName(int index) => index > 0 ? "<unused>" : "Output";

    public override void DrawProperties()
    {
        const float moduleWidth = 100f;

        float value = (float)mValue;
        ImGui.SetNextItemWidth(moduleWidth);

        if (ImGui.InputFloat("##value", ref value))
        {
            mValue = value;
        }
    }

    public override void Process(IReadOnlyList<IAudioInput?> inputs, IReadOnlyList<IAudioOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        var mainOutput = outputs[0];
        if (mainOutput is null)
        {
            return;
        }

        var signal = new StereoSignal<double>(channels, samplesRequested);
        for (int i = 0; i < channels; i++)
        {
            for (int j = 0; j < samplesRequested; j++)
            {
                signal[i][j] = mValue;
            }
        }

        mainOutput.PutAudio(signal);
    }
}

[RegisteredPlugin("Constant output")]
public sealed class ConstantOutputPlugin : Plugin
{
    public override Module Instantiate() => new ConstantOutputModule();
}
