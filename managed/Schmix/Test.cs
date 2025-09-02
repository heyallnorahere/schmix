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

    private static Plugin? FindPluginByNativeName(NativeString pluginName)
    {
        var name = pluginName.ToString();
        if (name is null)
        {
            return null;
        }

        Console.WriteLine(name);

        var plugin = Plugin.GetByName(name);
        if (plugin is null)
        {
            return null;
        }

        return plugin;
    }

    public Test(string pluginName)
    {
        var plugin = FindPluginByNativeName(pluginName);
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

        var outputs = new IAudioOutput[]
        {
            mOutput
        };

        mModule.Process(Array.Empty<IAudioInput>(), outputs, SampleRate, ChunkSize, Channels);

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
