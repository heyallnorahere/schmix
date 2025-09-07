namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Extension;
using Schmix.UI;

using System;
using System.Collections.Generic;
using System.Numerics;

internal sealed class OutputModule : Module
{
    public OutputModule()
    {
        uint id = OutputDevice.Default;
        mOutput = new OutputDevice(id, Rack.SampleRate, Rack.Channels);

        mDisplayedSignal = null;
    }

    protected override void Cleanup(bool disposed)
    {
        mOutput.Dispose();
    }

    public int ChunkSize => mOutput.SampleRate / 30;

    public override int SamplesRequested
    {
        get
        {
            if (mOutput is null)
            {
                return 0;
            }

            int queued = mOutput.QueuedSamples;
            int chunkSize = ChunkSize;

            if (queued < mOutput.SampleRate / 4)
            {
                return chunkSize;
            }

            return 0;
        }
    }

    public override int InputCount => 1;

    public override string Name => "Output device";

    public override string GetInputName(int index) => index > 0 ? "<unused>" : "Audio";

    public override void DrawProperties()
    {
        ImGui.PushItemWidth(150f);

        int channels = mOutput.Channels;
        int chunkSize = ChunkSize;

        for (int i = 0; i < channels; i++)
        {
            var samples = new float[chunkSize];
            for (int j = 0; j < chunkSize; j++)
            {
                samples[j] = (float)(mDisplayedSignal?[i][j] ?? 0);
            }

            ImGui.PlotLines($"##channel-{i}", ref samples[0], chunkSize, 0, $"Channel {i + 1}", -1f, 1f, Vector2.UnitY * 80f);
        }

        ImGui.PopItemWidth();
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        var audioInput = inputs[0];
        var audio = audioInput?.Signal;

        mDisplayedSignal = audio?.Copy();
        if (audio is not null && !mOutput.PutAudio(audio))
        {
            throw new InvalidOperationException("Failed to send audio to output device!");
        }
    }

    private OutputDevice mOutput;
    private StereoSignal<double>? mDisplayedSignal;
}

[RegisteredPlugin("Output")]
public sealed class OutputPlugin : Plugin
{
    public override Module Instantiate() => new OutputModule();
}
