namespace Schmix.Audio;

using System;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct Note
{
    public byte ID;
    public byte Channel;
}

public static class MIDI
{
    public static event Action<Note, double, DateTime>? OnNoteBegin;
    public static event Action<Note, DateTime>? OnNoteEnd;

    private static DateTime? sLastEvent = null;

    internal static DateTime ToTimestamp(long nanoseconds)
    {
        var offset = TimeSpan.FromTicks(nanoseconds / 100);
        var baseTimestamp = sLastEvent ?? DateTime.Now;

        return baseTimestamp + offset;
    }

    internal static unsafe void NoteBegin(Note* note, double velocity, long nanoseconds)
    {
        var timestamp = ToTimestamp(nanoseconds);
        OnNoteBegin?.Invoke(*note, velocity, timestamp);
    }

    internal static unsafe void NoteEnd(Note* note, long nanoseconds)
    {
        var timestamp = ToTimestamp(nanoseconds);
        OnNoteEnd?.Invoke(*note, timestamp);
    }

    internal static void ResetTime()
    {
        sLastEvent = DateTime.Now;
    }
}
