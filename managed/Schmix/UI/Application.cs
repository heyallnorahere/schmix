namespace Schmix.UI;

using Coral.Managed.Interop;

using ImGuiNET;

using Schmix.Core;
using Schmix.Extension;

using System.Numerics;

public static class Application
{
    private static bool sShowDemo = true;
    private static bool sShowDockspace = true;
    private static bool sShowRack = true;

    internal static bool Init()
    {
        Log.Info("Initializing rack...");

        Rack.Channels = 2;
        Rack.SampleRate = 40960;

        return true;
    }

    internal static void Shutdown()
    {
        Log.Debug("Managed application shutting down");

        Rack.Clear();
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

        Rack.Update();
    }

    private static void Dockspace(ref bool show)
    {
        if (!show)
        {
            return;
        }

        var viewport = ImGui.GetMainViewport();
        ImGui.SetNextWindowPos(viewport.WorkPos);
        ImGui.SetNextWindowSize(viewport.WorkSize);
        ImGui.SetNextWindowViewport(viewport.ID);

        ImGuiWindowFlags flags = 0;
        flags |= ImGuiWindowFlags.NoDocking;
        flags |= ImGuiWindowFlags.NoTitleBar | ImGuiWindowFlags.NoCollapse | ImGuiWindowFlags.NoResize;
        flags |= ImGuiWindowFlags.NoMove | ImGuiWindowFlags.NoBringToFrontOnFocus | ImGuiWindowFlags.NoNavFocus;

        ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, Vector2.Zero);
        ImGui.PushStyleVar(ImGuiStyleVar.WindowRounding, 0f);
        ImGui.PushStyleVar(ImGuiStyleVar.WindowBorderSize, 0f);

        ImGui.Begin("Dockspace", ref show, flags);
        ImGui.PopStyleVar(3);

        var io = ImGui.GetIO();
        if (io.ConfigFlags.HasFlag(ImGuiConfigFlags.DockingEnable))
        {
            uint id = ImGui.GetID("main-dockspace");
            ImGui.DockSpace(id);
        }
        else
        {
            ImGui.Text("Docking is disabled.");
        }

        ImGui.End();
    }

    private static bool Render(ImGuiInstance instance)
    {
        if (!instance.NewFrame())
        {
            return false;
        }

        instance.MakeContextCurrent();

        Dockspace(ref sShowDockspace);
        Rack.Render(ref sShowRack);

        if (sShowDemo)
        {
            ImGui.ShowDemoWindow(ref sShowDemo);
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
