namespace Schmix.UI;

using Coral.Managed.Interop;

using ImGuiNET;

using Schmix.Core;

using System.Runtime.InteropServices;

public sealed class ImGuiInstance : RefCounted
{
    internal static void GetAllocatorFunctions(out nint alloc, out nint free)
    {
        unsafe
        {
            void* allocPtr, freePtr;
            GetAllocatorFunctions_Impl(&allocPtr, &freePtr);

            alloc = (nint)allocPtr;
            free = (nint)freePtr;
        }
    }

    internal unsafe ImGuiInstance(void* address) : base(address)
    {
    }

    public void MakeContextCurrent()
    {
        GetAllocatorFunctions(out nint alloc, out nint free);
        ImGui.SetAllocatorFunctions(alloc, free);

        nint context = Context;
        ImGui.SetCurrentContext(context);
    }

    public unsafe bool NewFrame() => NewFrame_Impl(mAddress);
    public unsafe bool RenderAndPresent() => RenderAndPresent_Impl(mAddress);

    public unsafe nint Context => (nint)GetContext_Impl(mAddress);

    internal static unsafe delegate*<void**, void**, void> GetAllocatorFunctions_Impl = null;
    internal static unsafe delegate*<void*, void*> GetContext_Impl = null;

    internal static unsafe delegate*<void*, Bool32> NewFrame_Impl = null;
    internal static unsafe delegate*<void*, Bool32> RenderAndPresent_Impl = null;
}
