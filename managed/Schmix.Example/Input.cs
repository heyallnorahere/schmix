namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Core;
using Schmix.Extension;
using Schmix.UI;

using System.Collections.Generic;

internal sealed class InputModule : Module
{
    public InputModule()
    {
        mSelectedID = AudioDevice.Dummy;
        mInput = new AudioDevice(mSelectedID, Rack.SampleRate, Rack.Channels);
    }

    protected override void Cleanup(bool disposed)
    {
        if (disposed)
        {
            mInput.Dispose();
        }
    }

    public override string Name => "Input device";

    public override int OutputCount => 1;

    public override string GetOutputName(int index) => index > 0 ? "<unused>" : "Audio";

    public override void DrawProperties()
    {
        var inputDevices = AudioDevice.InputDevices;
        var currentName = mSelectedID == AudioDevice.Dummy ? "--Select--" : inputDevices[mSelectedID];

        ImGui.SetNextItemWidth(100f);
        if (!ImGui.BeginCombo("Device", currentName))
        {
            return;
        }

        foreach ((uint id, var name) in inputDevices)
        {
            bool isSelected = mSelectedID == id;
            if (ImGui.Selectable($"{name}##device-{id}", isSelected))
            {
                mSelectedID = id;
            }

            if (isSelected)
            {
                ImGui.SetItemDefaultFocus();
            }
        }

        ImGui.EndCombo();
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        if (mInput.DeviceID != mSelectedID || mInput.SampleRate != sampleRate || mInput.Channels != channels)
        {
            var previousInput = mInput;
            mInput = new AudioDevice(mSelectedID, sampleRate, channels);

            previousInput.Dispose();
        }

        int available = mInput.AvailableSamples;
        if (available < samplesRequested)
        {
            return;
        }

        if (!mInput.Flush())
        {
            Log.Error("Failed to flush input device!");
            return;
        }

        var received = mInput.GetAudio(samplesRequested);
        if (received is null)
        {
            Log.Error("Failed to retrieve sample from input device!");
            return;
        }

        StereoSignal<double> result;
        if (received.Length < samplesRequested)
        {
            result = new StereoSignal<double>(channels, samplesRequested);

            int offset = samplesRequested - received.Length;
            for (int i = 0; i < received.Length; i++)
            {
                for (int j = 0; j < channels; j++)
                {
                    result[j][offset + i] = received[j][i];
                }
            }
        }
        else
        {
            result = received;
        }

        outputs[0]?.PutSignal(result);
    }

    private AudioDevice mInput;
    private uint mSelectedID;
}

[RegisteredPlugin("Input")]
public sealed class InputPlugin : Plugin
{
    public override Module Instantiate() => new InputModule();
}
