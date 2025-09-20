namespace Schmix.Encoding;

using System;
using System.IO;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
internal unsafe struct NativeStreamCallbacks
{
    public delegate*<void*, int, void*, int> ReadPacket;
    public delegate*<void*, int, void*, int> WritePacket;
    public delegate*<long, SeekOrigin, void*, long> Seek;

    public void* UserData;
}

internal sealed class NativeStreamInterface : IDisposable
{
    public NativeStreamInterface(Stream stream)
    {
        mStream = stream;
        mStreamHandle = GCHandle.Alloc(stream);
    }

    ~NativeStreamInterface()
    {
        if (!mDisposed)
        {
            Dispose(false);
        }
    }

    public void Dispose()
    {
        if (mDisposed)
        {
            return;
        }

        Dispose(true);
        GC.SuppressFinalize(this);

        mDisposed = true;
    }

    private void Dispose(bool disposing)
    {
        // we dont care about the stream. only about the handle

        mStreamHandle.Free();
    }

    private static unsafe int ReadPacketFromStream(void* buffer, int bufferSize, void* userData)
    {
        var streamHandle = GCHandle.FromIntPtr((nint)userData);
        var stream = (Stream?)streamHandle.Target;

        if (stream is null)
        {
            return -1;
        }

        var span = new Span<byte>(buffer, bufferSize);
        try
        {
            return stream.Read(span);
        }
        catch (Exception)
        {
            return -1;
        }
    }

    private static unsafe int WritePacketToStream(void* data, int dataSize, void* userData)
    {
        var streamHandle = GCHandle.FromIntPtr((nint)userData);
        var stream = (Stream?)streamHandle.Target;

        if (stream is null)
        {
            return -1;
        }

        var span = new ReadOnlySpan<byte>(data, dataSize);
        try
        {
            stream.Write(span);
            return dataSize;
        }
        catch (Exception)
        {
            return -1;
        }
    }

    private static unsafe long Seek(long offset, SeekOrigin origin, void* userData)
    {
        var streamHandle = GCHandle.FromIntPtr((nint)userData);
        var stream = (Stream?)streamHandle.Target;

        if (stream is null)
        {
            return -1;
        }

        try
        {
            return stream.Seek(offset, origin);
        }
        catch (Exception)
        {
            return -1;
        }
    }

    public unsafe NativeStreamCallbacks GenerateCallbacks()
    {
        var callbacks = new NativeStreamCallbacks
        {
            ReadPacket = mStream.CanRead ? &ReadPacketFromStream : null,
            WritePacket = mStream.CanWrite ? &WritePacketToStream : null,
            Seek = mStream.CanSeek ? &Seek : null,

            UserData = (void*)GCHandle.ToIntPtr(mStreamHandle)
        };

        return callbacks;
    }

    private bool mDisposed;

    private readonly Stream mStream;
    private readonly GCHandle mStreamHandle;
}

public static class IO
{
    public enum Mode : int
    {
        Input = 0,
        Output
    }
}
