namespace Schmix.Encoding;

using System;
using System.IO;

public sealed class CodecStream : Stream
{
    public CodecStream(Stream source, IO.Mode mode, CodecParameters parameters, int streamIndex)
    {
        mSource = source;
        mSourceInterface = new NativeStreamInterface(source);

        var callbacks = mSourceInterface.GenerateCallbacks();
        unsafe
        {
            mAddress = ctor_Impl(&callbacks, mode, parameters.Address, streamIndex);

            if (mAddress is null)
            {
                throw new SystemException("Failed to open codec stream!");
            }
        }
    }

    protected override void Dispose(bool disposing)
    {
        unsafe
        {
            Close_Impl(mAddress);
        }

        if (disposing)
        {
            mSourceInterface.Dispose();

            if (mCloseSource)
            {
                mSource.Dispose();
            }
        }
    }

    public override long Length => 0;
    public override long Position
    {
        get => 0;
        set => throw new NotSupportedException();
    }

    public override void SetLength(long value) => throw new NotSupportedException();

    public override long Seek(long offset, SeekOrigin origin) => throw new NotSupportedException();

    private readonly Stream mSource;
    private bool mCloseSource;

    private readonly unsafe void* mAddress;
    private readonly NativeStreamInterface mSourceInterface;

    internal static unsafe delegate*<NativeStreamCallbacks*, IO.Mode, void*, int, void*> ctor_Impl = null;
    internal static unsafe delegate*<void*, void> Close_Impl = null;
}
