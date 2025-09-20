namespace Schmix.Example;

using Schmix.Audio;
using Schmix.Core;
using Schmix.Extension;

using System;
using System.Collections.Generic;

internal sealed class SingleNoteMIDIModule : Module
{
    public SingleNoteMIDIModule()
    {
        mPrevCV = mCV = 0;
        mCurrentNote = -1;
        mStart = mEnd = null;
        mActive = false;

        MIDI.OnNoteBegin += OnNoteBegin;
        MIDI.OnNoteEnd += OnNoteEnd;
    }

    protected override void Cleanup(bool disposed)
    {
        MIDI.OnNoteBegin -= OnNoteBegin;
        MIDI.OnNoteEnd -= OnNoteEnd;
    }

    private static readonly IReadOnlyList<string> sNoteNames = new string[]
    {
        "C",
        "C#",
        "D",
        "D#",
        "E",
        "F",
        "F#",
        "G",
        "G#",
        "A",
        "A#",
        "B"
    };

    private static string NameFromID(int id)
    {
        int notesPerOctave = sNoteNames.Count;

        int noteIndex = id % notesPerOctave;
        int octaveIndex = (id / notesPerOctave) - 1;

        return sNoteNames[noteIndex] + octaveIndex.ToString();
    }

    private void OnNoteBegin(Note note, double velocity, DateTime timestamp)
    {
        int id = note.ID;
        int cvID = id - 60; // relative to middle c

        mPrevCV = mCV;
        mCV = (double)cvID / (double)sNoteNames.Count;
        mCurrentNote = id;
        mStart = timestamp;
        mEnd = null;

        Log.Trace($"{NameFromID(id)} pressed at velocity {velocity}");
    }

    private void OnNoteEnd(Note note, DateTime timestamp)
    {
        int id = note.ID;
        if (id == mCurrentNote)
        {
            mEnd = timestamp;
        }

        Log.Trace($"{NameFromID(id)} released");
    }

    private const int PulseOutput = 0;
    private const int GateOutput = 1;
    private const int CVOutput = 2;

    public override int OutputCount => 3;

    public override string GetOutputName(int index) => index switch
    {
        PulseOutput => "Pulse",
        GateOutput => "Gate",
        CVOutput => "CV",
        _ => "<unused>"
    };

    public override string Name => "Single note MIDI";

    private double GetGate(DateTime now, out bool isActive)
    {
        isActive = false;
        if (mStart is null || mStart.Value > now)
        {
            return 0;
        }

        if (mEnd is not null && mEnd.Value < now)
        {
            return 0;
        }

        isActive = true;
        return 1;
    }

    private double GetCV(DateTime now)
    {
        if (mStart is not null && now < mStart.Value)
        {
            return mPrevCV;
        }

        return mCV;
    }

    public override void Process(IReadOnlyList<ISignalInput?> inputs, IReadOnlyList<ISignalOutput?> outputs, int sampleRate, int samplesRequested, int channels)
    {
        var chunkStart = DateTime.Now;
        var sampleSpan = TimeSpan.FromSeconds(1.0 / (double)sampleRate);

        var pulseSignal = new StereoSignal<double>(channels, samplesRequested);
        var gateSignal = new StereoSignal<double>(channels, samplesRequested);
        var cvSignal = new StereoSignal<double>(channels, samplesRequested);

        for (int i = 0; i < samplesRequested; i++)
        {
            var now = chunkStart + sampleSpan * i;

            double gate = GetGate(now, out bool isActive);
            double cv = GetCV(now);

            double pulse = 0;
            if (!mActive && isActive)
            {
                pulse = 1;
            }

            for (int j = 0; j < channels; j++)
            {
                pulseSignal[j][i] = pulse;
                gateSignal[j][i] = gate;
                cvSignal[j][i] = cv;
            }

            mActive = isActive;
        }

        outputs[PulseOutput]?.PutSignal(pulseSignal);
        outputs[GateOutput]?.PutSignal(gateSignal);
        outputs[CVOutput]?.PutSignal(cvSignal);
    }

    private double mPrevCV, mCV;
    private int mCurrentNote;
    private DateTime? mStart, mEnd;
    private bool mActive;
}

[RegisteredPlugin("Single note MIDI")]
public sealed class SingleNodeMIDIPlugin : Plugin
{
    public override Module Instantiate() => new SingleNoteMIDIModule();
}
