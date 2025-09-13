namespace Schmix.Audio;

using Coral.Managed.Interop;

using System;
using System.IO;

public sealed class EncodingStream : Stream
{
    public enum Codec : int
    {
        MP3 = 0,
        OGG
    }

    public enum StreamAction : int
    {
        Encoding = 0,
        Decoding
    }

    public enum SampleFormat : int
    {
        U8 = 0,
        S16,
        S32,
        Float,
        Double
    }

    public EncodingStream(Codec codec, StreamAction action, int channels, int sampleRate, SampleFormat sampleFormat)
    {
        unsafe
        {
            mEncoder = ctor_Impl(codec, action, channels, sampleRate, sampleFormat);
            if (mEncoder is null)
            {
                throw new SystemException("Failed to create encoding stream!");
            }
        }

        mCurrentChunk = Array.Empty<byte>();
        mChunkCursor = 0;
    }

    protected override void Dispose(bool disposing)
    {
        unsafe
        {
            Delete_Impl(mEncoder);
        }
    }

    public override int Read(Span<byte> buffer)
    {
        int offset = 0;
        while (offset < buffer.Length)
        {
            if (mChunkCursor >= mCurrentChunk.Length)
            {
                NativeArray<byte> newChunk;
                unsafe
                {
                    if (!GetChunk_Impl(mEncoder, &newChunk))
                    {
                        throw new SystemException("Failed to get chunk from encoding stream!");
                    }
                }

                mCurrentChunk = newChunk.ToArray();
                mChunkCursor = 0;

                if (mCurrentChunk.Length == 0)
                {
                    break;
                }
            }

            var bufferSlice = buffer.Slice(offset);
            int bytesAvailable = mCurrentChunk.Length - mChunkCursor;
            int bytesCopied = int.Min(bufferSlice.Length, bytesAvailable);

            var sourceSlice = mCurrentChunk.AsSpan().Slice(mChunkCursor, bytesCopied);
            sourceSlice.CopyTo(bufferSlice);

            offset += bytesCopied;
            mChunkCursor += bytesCopied;
        }

        return offset;
    }

    public override int Read(byte[] buffer, int offset, int count)
    {
        var span = buffer.AsSpan();
        var slice = span.Slice(offset, count);

        return Read(slice);
    }

    public override void Write(ReadOnlySpan<byte> buffer)
    {
        using var chunk = new NativeArray<byte>(buffer.ToArray());

        unsafe
        {
            if (!PutChunk_Impl(mEncoder, chunk))
            {
                throw new SystemException("Failed to send chunk to encoding stream!");
            }
        }
    }

    public override void Write(byte[] buffer, int offset, int count)
    {
        var span = buffer.AsSpan();
        var slice = span.Slice(offset, count);

        Write(slice);
    }

    public override long Position
    {
        get => throw new NotSupportedException();
        set => throw new NotSupportedException();
    }

    public override long Length => throw new NotSupportedException();

    public override void SetLength(long value) => throw new NotSupportedException();

    public override void Flush()
    {
        // does nothing
    }

    public override long Seek(long offset, SeekOrigin origin) => throw new NotSupportedException();

    public override bool CanRead => true;
    public override bool CanWrite => true;
    public override bool CanSeek => false;
    public override bool CanTimeout => false;

    public unsafe Codec CodecID => GetCodecID_Impl(mEncoder);
    public unsafe StreamAction Action => GetAction_Impl(mEncoder);

    public unsafe int Channels => GetChannels_Impl(mEncoder);
    public unsafe int SampleRate => GetSampleRate_Impl(mEncoder);
    public unsafe SampleFormat Format => GetSampleFormat_Impl(mEncoder);

    private readonly unsafe void* mEncoder;

    private byte[] mCurrentChunk;
    private int mChunkCursor;

    internal static unsafe delegate*<Codec, StreamAction, int, int, SampleFormat, void*> ctor_Impl = null;
    internal static unsafe delegate*<void*, void> Delete_Impl = null;

    internal static unsafe delegate*<void*, NativeArray<byte>*, Bool32> GetChunk_Impl = null;
    internal static unsafe delegate*<void*, NativeArray<byte>, Bool32> PutChunk_Impl = null;

    internal static unsafe delegate*<void*, Codec> GetCodecID_Impl = null;
    internal static unsafe delegate*<void*, StreamAction> GetAction_Impl = null;

    internal static unsafe delegate*<void*, int> GetChannels_Impl = null;
    internal static unsafe delegate*<void*, int> GetSampleRate_Impl = null;
    internal static unsafe delegate*<void*, SampleFormat> GetSampleFormat_Impl = null;
}
