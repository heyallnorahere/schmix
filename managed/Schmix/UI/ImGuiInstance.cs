namespace Schmix.UI;

using Coral.Managed.Interop;

using ImGuiNET;
using imnodesNET;

using Schmix.Core;

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

        context = NodesContext;
        imnodesNative.imnodes_SetCurrentContext(context);
    }

    public unsafe bool NewFrame() => NewFrame_Impl(mAddress);
    public unsafe bool RenderAndPresent() => RenderAndPresent_Impl(mAddress);

    public unsafe nint Context => (nint)GetContext_Impl(mAddress);
    public unsafe nint NodesContext => (nint)GetNodesContext_Impl(mAddress);

    internal static unsafe delegate*<void**, void**, void> GetAllocatorFunctions_Impl = null;
    internal static unsafe delegate*<void*, void*> GetContext_Impl = null;
    internal static unsafe delegate*<void*, void*> GetNodesContext_Impl = null;

    internal static unsafe delegate*<void*, Bool32> NewFrame_Impl = null;
    internal static unsafe delegate*<void*, Bool32> RenderAndPresent_Impl = null;
}
