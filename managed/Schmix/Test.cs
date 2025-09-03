namespace Schmix;

using Coral.Managed.Interop;

using Schmix.Audio;
using Schmix.Extension;

using System;

internal sealed class Test : IDisposable
{
    private const int SampleRate = 40960;
    private const int Channels = 2;
    private const int ChunkSize = SampleRate / 4;

    public Test()
    {
        var plugin = Plugin.GetByName("Oscillator");
        if (plugin is null)
        {
            throw new ArgumentException("Invalid plugin name!");
        }

        mPlugin = plugin;
        mModule = plugin.Instantiate();

        uint deviceID = WindowAudioOutput.DefaultDeviceID;
        mOutput = new WindowAudioOutput(deviceID, SampleRate, Channels);
    }

    public bool Update()
    {
        int queued = mOutput.QueuedSamples;
        if (queued >= ChunkSize)
        {
            return true;
        }

        mOutput.ResetSignal();

        var inputs = new IAudioInput?[mModule.InputCount];
        var outputs = new IAudioOutput?[mModule.OutputCount];

        Array.Fill(inputs, null);
        Array.Fill(outputs, null);

        outputs[0] = mOutput;

        mModule.Process(inputs, outputs, SampleRate, ChunkSize, Channels);

        return mOutput.Flush();
    }

    ~Test()
    {
        if (mDisposed)
        {
            return;
        }

        DoDispose(false);
    }

    private void DoDispose(bool disposing)
    {
        if (disposing)
        {
            mOutput.Dispose();
            mModule.Dispose();
        }
    }

    public void Dispose()
    {
        if (mDisposed)
        {
            return;
        }

        DoDispose(true);
        GC.SuppressFinalize(this);

        mDisposed = true;
    }

    private readonly Plugin mPlugin;
    private readonly Module mModule;

    private readonly WindowAudioOutput mOutput;

    private bool mDisposed;
}
