namespace Schmix.Example;

using ImGuiNET;

using Schmix.Audio;
using Schmix.Extension;

using System;
using System.Collections.Generic;

internal enum EnvelopePhase
{
    Attack,
    Decay,
    Sustain,
    Release,
    Mute
}

internal struct EnvelopeStatus
{
    public double T;
    public EnvelopePhase Phase;

    public double Gain;
    public double StartGain, EndGain;
}

internal sealed class EnvelopeModule : Module
{
    public EnvelopeModule()
    {
        mAttack = TimeSpan.FromSeconds(0.25);
        mDecay = TimeSpan.FromSeconds(0.5);
        mRelease = TimeSpan.FromSeconds(0.25);
        mSustain = 0.75;

        mStatus = new List<EnvelopeStatus>();
    }

    public override int InputCount => 1;
    public override int OutputCount => 1;

    public override string GetInputName(int index) => index > 0 ? "<unused>" : "Gate";
    public override string GetOutputName(int index) => index > 0 ? "<unused>" : "CV";

    public override string Name => "Envelope";

    private static bool DrawTimeControl(string label, ref TimeSpan value)
    {
        float seconds = (float)value.TotalSeconds;
        if (ImGui.DragFloat(label, ref seconds, 0.1f))
        {
            value = TimeSpan.FromSeconds(seconds);
            return true;
        }

        return false;
    }

    public override void DrawProperties()
    {
        ImGui.PushItemWidth(100f);

        DrawTimeControl("Attack", ref mAttack);
        DrawTimeControl("Decay", ref mDecay);

        float gain = (float)mSustain;
        if (ImGui.SliderFloat("Sustain", ref gain, 0f, 1f))
        {
            mSustain = gain;
        }

        DrawTimeControl("Release", ref mRelease);

        ImGui.PopItemWidth();
    }

    private static double Lerp(double a, double b, double t) => (1 - t) * a + t * b;

    private void ProcessChannel(MonoSignal<double>? gateSignal, MonoSignal<double> cvSignal, ref EnvelopeStatus status, DateTime chunkStart, TimeSpan sampleSpan)
    {
        bool isNoteActive = false;
        bool phaseChanged = true;

        for (int i = 0; i < cvSignal.Length; i++)
        {
            if (phaseChanged)
            {
                isNoteActive = status.Phase < EnvelopePhase.Release;
                phaseChanged = false;
            }

            double gate = gateSignal?[i] ?? 0;
            bool isGateOpen = gate >= 0.1;

            if (isGateOpen && !isNoteActive)
            {
                status.T = 0;
                status.Phase = EnvelopePhase.Attack;

                status.StartGain = status.Gain;

                phaseChanged = true;
            }
            else if (!isGateOpen && isNoteActive)
            {
                status.T = 0;
                status.Phase = EnvelopePhase.Release;

                status.EndGain = status.Gain;

                phaseChanged = true;
            }

            TimeSpan? phaseLength = status.Phase switch
            {
                EnvelopePhase.Attack => mAttack,
                EnvelopePhase.Decay => mDecay,
                EnvelopePhase.Release => mRelease,
                _ => null
            };

            if (phaseLength is not null)
            {
                status.T += sampleSpan / phaseLength.Value;
                if (phaseLength is not null && status.T >= 1)
                {
                    status.Phase++;
                    status.T = 0;

                    phaseChanged = true;
                }
            }

            double gain = status.Phase switch
            {
                EnvelopePhase.Attack => Lerp(status.StartGain, 1, status.T),
                EnvelopePhase.Decay => Lerp(1, mSustain, status.T),
                EnvelopePhase.Sustain => mSustain,
                EnvelopePhase.Release => Lerp(status.EndGain, 0, status.T),
                _ => 0
            };

            status.Gain = gain;
            cvSignal[i] = Math.Log2(gain);
        }
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        var chunkStart = DateTime.Now;
        var sampleSpan = TimeSpan.FromSeconds(1.0 / (double)sampleRate);

        var gateInput = inputs[0];
        var cvOutput = outputs[0];

        if (cvOutput is null)
        {
            return;
        }

        var gateSignal = gateInput?.Signal;
        var cvSignal = new StereoSignal<double>(channels, samplesRequested);

        for (int i = 0; i < channels; i++)
        {
            EnvelopeStatus status;
            if (i < mStatus.Count)
            {
                status = mStatus[i];
            }
            else
            {
                status = new EnvelopeStatus
                {
                    Phase = EnvelopePhase.Mute
                };
            }

            ProcessChannel(gateSignal?[i], cvSignal[i], ref status, chunkStart, sampleSpan);
            if (i < mStatus.Count)
            {
                mStatus[i] = status;
            }
            else
            {
                mStatus.Add(status);
            }
        }

        cvOutput.PutSignal(cvSignal);
    }

    private TimeSpan mAttack, mDecay, mRelease;
    private double mSustain;

    private readonly List<EnvelopeStatus> mStatus;
}

[RegisteredPlugin("Envelope")]
public sealed class EnvelopePlugin : Plugin
{
    public override Module Instantiate() => new EnvelopeModule();
}
