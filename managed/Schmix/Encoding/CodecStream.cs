namespace Schmix.Encoding;

using Coral.Managed.Interop;

using Schmix.Core;

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

    internal unsafe IO.Mode GetMode() => GetMode_Impl(mAddress);
    internal unsafe CodecParameters GetParameters() => new CodecParameters(GetParameters_Impl(mAddress));
    internal unsafe int GetStreamIndex() => GetStreamIndex_Impl(mAddress);
    internal unsafe int GetFrameSize() => GetFrameSize_Impl(mAddress);

    public IO.Mode Mode => GetMode();
    public CodecParameters Parameters => GetParameters();
    public int StreamIndex => GetStreamIndex();

    private unsafe void ReadFrame()
    {
        void* data = null;
        int samples = ReadFrame_Impl(mAddress, &data);

        if (samples < 0)
        {
            mFrameBuffer = null;

            // assume eof
            throw new EndOfStreamException();
        }

        var parameters = GetParameters();
        int bufferSize = parameters.CalculateFrameBufferSize(samples);

        var source = new Span<byte>(data, bufferSize);
        mFrameBuffer = new byte[bufferSize];
        source.CopyTo(mFrameBuffer);

        MemoryAllocator.Free(data);
    }

    public override int Read(Span<byte> buffer)
    {
        var cursor = buffer;

        do
        {
        }
        while (mFrameBuffer is not null && mFrameBuffer.Length > 0 && cursor.Length > 0);

        return 0;
    }

    public override int Read(byte[] buffer, int offset, int count)
    {
        var span = buffer.AsSpan().Slice(offset, count);
        return Read(span);
    }

    public override void SetLength(long value) => throw new NotSupportedException();

    public override long Seek(long offset, SeekOrigin origin) => throw new NotSupportedException();

    private byte[]? mFrameBuffer;
    private int mCursor;

    private readonly MemoryStream? mOutputBuffer;

    private readonly Stream mSource;
    private bool mCloseSource;

    private readonly unsafe void* mAddress;
    private readonly NativeStreamInterface mSourceInterface;

    internal static unsafe delegate*<NativeStreamCallbacks*, IO.Mode, void*, int, void*> ctor_Impl = null;
    internal static unsafe delegate*<void*, void> Close_Impl = null;

    internal static unsafe delegate*<void*, IO.Mode> GetMode_Impl = null;
    internal static unsafe delegate*<void*, void*> GetParameters_Impl = null;
    internal static unsafe delegate*<void*, int> GetStreamIndex_Impl = null;
    internal static unsafe delegate*<void*, int> GetFrameSize_Impl = null;

    internal static unsafe delegate*<void*, void**, int> ReadFrame_Impl = null;
    internal static unsafe delegate*<void*, void*, int, Bool32> WriteFrame_Impl = null;
    internal static unsafe delegate*<void*, Bool32> Flush_Impl = null;
}
