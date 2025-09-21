namespace Schmix.Encoding;

using System;

public readonly unsafe struct CodecParameters : IDisposable
{
    public CodecParameters()
    {
        mAddress = New_Impl();
    }

    internal CodecParameters(void* source)
    {
        mAddress = source;
    }

    public void Dispose()
    {
        Delete_Impl(mAddress);
    }

    public CodecParameters Duplicate()
    {
        void* address = Duplicate_Impl(mAddress);
        return new CodecParameters(address);
    }

    internal void* Address => mAddress;
    public void* Raw => GetRaw_Impl(mAddress);

    public int Channels => GetChannels_Impl(mAddress);

    public unsafe int CalculateFrameBufferSize(int samples) => CalculateFrameBufferSize_Impl(mAddress, samples);

    private readonly void* mAddress;

    internal static readonly delegate*<void*> New_Impl = null;
    internal static readonly delegate*<void*, void> Delete_Impl = null;
    internal static readonly delegate*<void*, void*> Duplicate_Impl = null;

    internal static readonly delegate*<void*, void*> GetRaw_Impl = null;

    internal static readonly delegate*<void*, int> GetChannels_Impl = null;

    internal static readonly delegate*<void*, int, int> CalculateFrameBufferSize_Impl = null;
}
