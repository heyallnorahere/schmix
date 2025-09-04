namespace Schmix;

using ImGuiNET;
using imnodesNET;

using Schmix.Audio;
using Schmix.Extension;

using System;
using System.Collections.Generic;

internal sealed class Test : IDisposable
{
    private static readonly float[] sValues = new float[4];
    private static readonly HashSet<int> sLinks = new HashSet<int>();

    public static void NodeTest(ref bool show)
    {
        if (!show)
        {
            return;
        }

        ImGui.Begin("Nodes", ref show);
        imnodes.PushAttributeFlag(AttributeFlags.EnableLinkDetachWithDragClick);
        imnodes.BeginNodeEditor();

        for (int i = 1; i <= sValues.Length; i++)
        {
            imnodes.BeginNode(i);

            imnodes.BeginNodeTitleBar();
            ImGui.TextUnformatted($"Test node #{i}");
            imnodes.EndNodeTitleBar();

            imnodes.BeginInputAttribute(i << 8);
            ImGui.TextUnformatted("Input");
            imnodes.EndInputAttribute();

            imnodes.BeginStaticAttribute(i << 16);
            ImGui.SameLine();
            ImGui.PushItemWidth(120f);
            ImGui.DragFloat("Value", ref sValues[i - 1], 0.01f);
            ImGui.PopItemWidth();
            imnodes.EndStaticAttribute();

            imnodes.BeginOutputAttribute(i << 24);
            ImGui.SameLine();
            ImGui.TextUnformatted("Output");
            imnodes.EndOutputAttribute();

            imnodes.EndNode();
        }

        int start, end;
        foreach (int link in sLinks)
        {
            start = (int)(link & 0x0000FF00);
            end = (int)(link & 0xFF000000);

            imnodes.Link(link, start, end);
        }

        imnodes.EndNodeEditor();

        start = end = 0;
        if (imnodes.IsLinkCreated(ref start, ref end))
        {
            int link = start | end;
            sLinks.Add(link);
        }

        int destroyedLink = 0;
        if (imnodes.IsLinkDestroyed(ref destroyedLink))
        {
            sLinks.Remove(destroyedLink);
        }

        imnodes.PopAttributeFlag();
        ImGui.End();
    }

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
