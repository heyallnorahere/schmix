namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Core;
using Schmix.Extension;

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;

internal sealed class SoundModule : Module
{
    public SoundModule()
    {
        mChannels = mSampleRate = 0;
        mSelectedPath = string.Empty;

        mPulseActive = false;
        ResetSound();
    }

    public override int InputCount => 1;
    public override int OutputCount => 1;

    public override string GetInputName(int index) => index > 0 ? "<unused>" : "Pulse";
    public override string GetOutputName(int index) => index > 0 ? "<unused>" : "Audio";

    public override string Name => "Sound";

    public static double[] LoadSound(string path, int channels, int sampleRate)
    {
        const EncodingStream.StreamAction action = EncodingStream.StreamAction.Decoding;
        const EncodingStream.SampleFormat format = EncodingStream.SampleFormat.Double;

        var codecID = EncodingStream.GuessCodec(path);
        using var encodingStream = new EncodingStream(codecID, action, channels, sampleRate, format);

        using var fileStream = new FileStream(path, FileMode.Open, FileAccess.Read);
        using var pcmStream = new MemoryStream();

        fileStream.CopyTo(encodingStream);
        encodingStream.CopyTo(pcmStream);

        var data = pcmStream.GetBuffer();
        var samples = MemoryMarshal.Cast<byte, double>(data);

        return samples.ToArray();
    }

    [MemberNotNull(nameof(mPath))]
    [MemberNotNull(nameof(mData))]
    [MemberNotNull(nameof(mCursor))]
    public void ResetSound()
    {
        Log.Debug("Resetting loaded sound");

        mPath = string.Empty;

        mData = Array.Empty<double>();
        mCursor = 0;
    }

    private void LoadSoundFromSelectedPath()
    {
        Log.Debug($"Loading sound {mSelectedPath} for {mChannels} channels sampled at {mSampleRate} Hz");

        try
        {
            mData = LoadSound(mSelectedPath, mChannels, mSampleRate);
            mCursor = mData.Length;

            mPath = mSelectedPath;
        }
        catch (Exception ex)
        {
            Log.Error($"Failed to load sound {mSelectedPath}: {ex}");
            mSelectedPath = mPath;
        }
    }

    public override void DrawProperties()
    {
        const float moduleWidth = 100f;
        ImGui.PushItemWidth(moduleWidth);

        var style = ImGui.GetStyle();
        float spacing = style.ItemSpacing.X;

        if (ImGui.InputText("##path-input", ref mSelectedPath, 256, ImGuiInputTextFlags.EnterReturnsTrue))
        {
            LoadSoundFromSelectedPath();
        }

        float buttonWidth = (moduleWidth - spacing) / 2;
        if (ImGui.Button("Load", Vector2.UnitX * buttonWidth))
        {
            LoadSoundFromSelectedPath();
        }

        ImGui.SameLine(buttonWidth + spacing);
        ImGui.BeginDisabled(mData.Length > 0);

        if (ImGui.Button("Reset", Vector2.UnitX * buttonWidth))
        {
            ResetSound();
        }

        ImGui.EndDisabled();

        if (mData.Length > 0)
        {
            var floatData = mData.Select(sample => (float)sample).ToArray();
            for (int i = 0; i < mChannels; i++)
            {
                ImGui.PlotLines($"##channel-{i}", ref floatData[0], floatData.Length / mChannels, 0,
                        string.Empty, -1f, 1f, Vector2.UnitY * 80f,
                        sizeof(float) * mChannels);
            }
        }

        ImGui.PopItemWidth();
    }

    private void ReloadSound()
    {
        Log.Debug($"Reloading sound {mPath} for {mChannels} channels sampled at {mSampleRate} Hz");

        try
        {
            mData = LoadSound(mPath, mChannels, mSampleRate);
            mCursor = mData.Length;
        }
        catch (Exception ex)
        {
            Log.Error($"Failed to reload sound {mPath}: {ex}");
            ResetSound();
        }
    }

    public bool PlaySound()
    {
        if (mData.Length == 0)
        {
            return false;
        }

        Log.Trace($"Playing sound {mPath}");
        mCursor = 0;

        return true;
    }

    private void ProcessPulse(double pulse)
    {
        bool wasPulseActive = mPulseActive;
        mPulseActive = pulse > 0.1;

        // rising edge
        if (mPulseActive && !wasPulseActive)
        {
            Log.Trace("Rising edge detected from pulse input");
            PlaySound();
        }
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        if (mChannels != channels || mSampleRate != sampleRate)
        {
            mChannels = channels;
            mSampleRate = sampleRate;

            if (mPath.Length > 0)
            {
                ReloadSound();
            }
        }

        // we only care about one channel really
        var pulseSignal = inputs[0]?.Signal?[0];

        var audioSignal = new StereoSignal<double>(channels, samplesRequested);
        for (int i = 0; i < samplesRequested; i++)
        {
            double pulse = pulseSignal?[i] ?? 0;
            ProcessPulse(pulse);

            for (int j = 0; j < channels; j++)
            {
                double sample = 0;
                if (mCursor < mData.Length)
                {
                    sample = mData[mCursor++];
                }

                audioSignal[j][i] = sample;
            }
        }

        outputs[0]?.PutSignal(audioSignal);
    }

    private string mSelectedPath;

    private string mPath;
    private double[] mData;
    private bool mPulseActive;
    private int mCursor;

    private int mChannels, mSampleRate;
}

[RegisteredPlugin("Sound")]
public sealed class SoundPlugin : Plugin
{
    public override Module Instantiate() => new SoundModule();
}
