namespace Schmix;

using System;

public abstract class RefCounted : IDisposable
{
    public RefCounted(nuint address)
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

        unsafe
        {
            AddRef_Impl(mAddress);
        }

        mCounted = true;
    }

    private void RemoveRef()
    {
        if (!mCounted)
        {
            return;
        }

        unsafe
        {
            RemoveRef_Impl(mAddress);
        }

        mCounted = false;
    }

    internal static unsafe delegate*<nuint, void> AddRef_Impl = null;
    internal static unsafe delegate*<nuint, void> RemoveRef_Impl = null;

    internal nuint NativeAddress => mAddress;

    protected readonly nuint mAddress;
    private bool mCounted;
}
