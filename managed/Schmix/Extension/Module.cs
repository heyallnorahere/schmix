namespace Schmix.Extension;

using Schmix.Audio;

using System;
using System.Collections.Generic;

public abstract class Module : IDisposable
{
    public enum InterfaceType
    {
        Signal,
        Note,
        Any,
        SameAsOpposing,
    }

    public readonly struct InterfaceSpec
    {
        private InterfaceSpec(InterfaceType type, int reference)
        {
            Type = type;
            Reference = reference;
        }

        public static InterfaceSpec Signal => new InterfaceSpec(InterfaceType.Signal, -1);
        public static InterfaceSpec Note => new InterfaceSpec(InterfaceType.Note, -1);
        public static InterfaceSpec Any => new InterfaceSpec(InterfaceType.Any, -1);

        public static InterfaceSpec SameAsOpposing(int reference) => new InterfaceSpec(InterfaceType.SameAsOpposing, reference);

        public readonly InterfaceType Type;
        public readonly int Reference;
    }

    public virtual string GetInputName(int index) => $"<input {index}>";
    public virtual string GetOutputName(int index) => $"<output {index}>";

    public virtual IReadOnlyList<InterfaceSpec> Inputs => Array.Empty<InterfaceSpec>();
    public virtual IReadOnlyList<InterfaceSpec> Outputs => Array.Empty<InterfaceSpec>();

    protected Module()
    {
        mDisposed = false;
    }

    ~Module()
    {
        if (mDisposed)
        {
            return;
        }

        Cleanup(false);
    }

    public void Dispose()
    {
        if (mDisposed)
        {
            return;
        }

        Cleanup(true);
        GC.SuppressFinalize(this);

        mDisposed = true;
    }

    protected virtual void Cleanup(bool disposed)
    {
    }

    public abstract void Process(IReadOnlyList<object?> inputs, IReadOnlyList<object?> outputs, int sampleRate, int samplesRequested, int channels);

    private bool mDisposed;
}
