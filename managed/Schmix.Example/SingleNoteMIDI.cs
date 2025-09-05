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
        mNextChunk = DateTime.Now;

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

    private const int GateOutput = 0;
    private const int CVOutput = 1;

    public override int OutputCount => 2;

    public override string GetOutputName(int index) => index switch
    {
        GateOutput => "Gate",
        CVOutput => "CV",
        _ => "<unused>"
    };

    public override string Name => "Single note MIDI";

    private double GetGate(DateTime now)
    {
        if (mStart is null || mStart.Value > now)
        {
            return 0;
        }

        if (mEnd is not null && mEnd.Value < now)
        {
            return 0;
        }

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

        var gateSignal = new StereoSignal<double>(channels, samplesRequested);
        var cvSignal = new StereoSignal<double>(channels, samplesRequested);

        for (int i = 0; i < samplesRequested; i++)
        {
            var now = chunkStart + sampleSpan * i;

            double gate = GetGate(now);
            double cv = GetCV(now);

            for (int j = 0; j < channels; j++)
            {
                gateSignal[j][i] = gate;
                cvSignal[j][i] = cv;
            }
        }

        outputs[GateOutput]?.PutSignal(gateSignal);
        outputs[CVOutput]?.PutSignal(cvSignal);

        mNextChunk = DateTime.Now;
    }

    private double mPrevCV, mCV;
    private int mCurrentNote;
    private DateTime? mStart, mEnd;
    private DateTime mNextChunk;
}

[RegisteredPlugin("Single note MIDI")]
public sealed class SingleNodeMIDIPlugin : Plugin
{
    public override Module Instantiate() => new SingleNoteMIDIModule();
}
