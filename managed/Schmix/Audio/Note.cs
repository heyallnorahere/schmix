namespace Schmix.Audio;

using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct Note
{
    public double Frequency;

    public bool Active;
    public double TimeActive, TimeInactive;
}
