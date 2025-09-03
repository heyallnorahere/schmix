namespace Schmix.UI;

using Coral.Managed.Interop;

using ImGuiNET;
using imnodesNET;

using Schmix.Core;

using System.Collections.Generic;

public static class Application
{
    private static Test? sTest = null;

    internal static bool Init()
    {
        Log.Info("Hello from managed runtime!");
        // sTest = new Test();

        return true;
    }

    internal static void Shutdown()
    {
        Log.Info("Tearing down managed interfaces...");
        sTest?.Dispose();
    }

    internal static void Update(double deltaTime)
    {
        using var instance = RefImGuiInstance();
        if (!Render(instance))
        {
            Log.Error("Failed to render ImGui. Exiting 1...");
            Quit(1);

            return;
        }

        if (sTest is not null && !sTest.Update())
        {
            Log.Error("Failed to update audio! Exiting 1...");
            Quit(1);

            return;
        }
    }

    private static bool sShowDemo = true;
    private static bool sShowNodes = true;

    private static readonly float[] sValues = new float[4];
    private static readonly HashSet<int> sLinks = new HashSet<int>();

    private static bool Render(ImGuiInstance instance)
    {
        if (!instance.NewFrame())
        {
            return false;
        }

        instance.MakeContextCurrent();

        if (sShowDemo)
        {
            ImGui.ShowDemoWindow(ref sShowDemo);

            if (!sShowDemo)
            {
                Log.Info("ImGui demo window closed");
            }
        }

        if (sShowNodes)
        {
            ImGui.Begin("Nodes", ref sShowNodes);
            imnodes.PushAttributeFlag(AttributeFlags.EnableLinkDetachWithDragClick);
            imnodes.BeginNodeEditor();

            for (int i = 0; i < sValues.Length; i++)
            {
                imnodes.BeginNode(i);

                imnodes.BeginNodeTitleBar();
                ImGui.TextUnformatted($"Test node #{i + 1}");
                imnodes.EndNodeTitleBar();

                imnodes.BeginInputAttribute(i << 8);
                ImGui.TextUnformatted("Input");
                imnodes.EndInputAttribute();

                imnodes.BeginStaticAttribute(i << 16);
                ImGui.PushItemWidth(120f);
                const string valueText = "Value";
                ImGui.DragFloat(valueText, ref sValues[i], 0.01f);
                ImGui.PopItemWidth();
                imnodes.EndStaticAttribute();

                imnodes.BeginOutputAttribute(i << 24);
                const string outputText = "Output";
                float textWidth = ImGui.CalcTextSize(outputText).X;
                ImGui.Indent(120f - ImGui.CalcTextSize(valueText).X - textWidth);
                ImGui.TextUnformatted(outputText);
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

        if (!instance.RenderAndPresent())
        {
            return false;
        }

        return true;
    }

    public static unsafe bool IsRunning => IsRunning_Impl();

    public static unsafe void Quit(int status = 0) => Quit_Impl(status);

    public static ImGuiInstance RefImGuiInstance()
    {
        unsafe
        {
            void* address = GetImGuiInstance_Impl();
            return new ImGuiInstance(address);
        }
    }

    internal static unsafe delegate*<Bool32> IsRunning_Impl = null;

    internal static unsafe delegate*<int, void> Quit_Impl = null;

    internal static unsafe delegate*<void*> GetImGuiInstance_Impl = null;
}
