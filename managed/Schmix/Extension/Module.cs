namespace Schmix.Extension;

using Schmix.Audio;

using System;
using System.Collections.Generic;

public abstract class Module : IDisposable
{
    public virtual string GetInputName(int index) => $"<input {index}>";
    public virtual string GetOutputName(int index) => $"<output {index}>";

    public virtual int InputCount => 0;
    public virtual int OutputCount => 0;

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

    public abstract void Process(IReadOnlyList<IAudioInput?> inputs, IReadOnlyList<IAudioOutput?> outputs, int sampleRate, int samplesRequested, int channels);

    private bool mDisposed;
}
