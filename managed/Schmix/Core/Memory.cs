namespace Schmix.Core;

public static unsafe class MemoryAllocator
{
    public static void* Allocate(nint size) => Allocate_Impl(size);
    public static void Free(void* block) => Free_Impl(block);

    internal static delegate*<nint, void*> Allocate_Impl = null;
    internal static delegate*<void*, void> Free_Impl = null;
}
