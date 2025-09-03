namespace Schmix.UI;

using Coral.Managed.Interop;

using ImGuiNET;

using Schmix.Core;

public static class Application
{
    private static Test? sTest = null;

    internal static bool Init()
    {
        Log.Info("Hello from managed runtime!");
        sTest = new Test();

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
