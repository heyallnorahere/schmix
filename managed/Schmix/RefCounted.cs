namespace Schmix;

using System;

public abstract unsafe class RefCounted : IDisposable
{
    internal RefCounted(void* address)
    {
        mAddress = address;
        mCounted = false;

        AddRef();
    }

    ~RefCounted()
    {
        RemoveRef();
    }

    public void Dispose()
    {
        RemoveRef();
        GC.SuppressFinalize(this);
    }

    private void AddRef()
    {
        if (mCounted)
        {
            return;
        }

        AddRef_Impl(mAddress);

        mCounted = true;
    }

    private void RemoveRef()
    {
        if (!mCounted)
        {
            return;
        }

        RemoveRef_Impl(mAddress);

        mCounted = false;
    }

    internal static unsafe delegate*<void*, void> AddRef_Impl = null;
    internal static unsafe delegate*<void*, void> RemoveRef_Impl = null;

    internal void* NativeAddress => mAddress;

    protected readonly void* mAddress;
    private bool mCounted;
}
