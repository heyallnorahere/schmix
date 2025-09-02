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
    }

    public virtual string GetInputName(int index) => $"<input {index}>";
    public virtual string GetOutputName(int index) => $"<output {index}>";

    public IReadOnlyList<InterfaceType> Inputs => Array.Empty<InterfaceType>();
    public IReadOnlyList<InterfaceType> Outputs => Array.Empty<InterfaceType>();

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

    public abstract void Process(IReadOnlyList<IAudioInput?> inputs, IReadOnlyList<IAudioOutput?> outputs);

    private bool mDisposed;
}
