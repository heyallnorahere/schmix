namespace Schmix.Core;

using Coral.Managed.Interop;

using System.Runtime.CompilerServices;

public static class Log
{
    public enum Level : int
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5
    }

    private static void Print_Internal(Level level, string msg, string memberName, string file, int line)
    {
        using NativeString msgNative = msg;
        using NativeString nameNative = memberName;
        using NativeString fileNative = file;

        unsafe
        {
            Print_Impl(level, msgNative, nameNative, fileNative, line);
        }
    }

    public static void Print(Level level, string msg, [CallerMemberName] string memberName = "", [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        Print_Internal(level, msg, memberName, file, line);
    }

    public static void Trace(string msg, [CallerMemberName] string memberName = "", [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        Print_Internal(Level.Trace, msg, memberName, file, line);
    }

    public static void Debug(string msg, [CallerMemberName] string memberName = "", [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        Print_Internal(Level.Debug, msg, memberName, file, line);
    }

    public static void Info(string msg, [CallerMemberName] string memberName = "", [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        Print_Internal(Level.Info, msg, memberName, file, line);
    }

    public static void Warn(string msg, [CallerMemberName] string memberName = "", [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        Print_Internal(Level.Warn, msg, memberName, file, line);
    }

    public static void Error(string msg, [CallerMemberName] string memberName = "", [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        Print_Internal(Level.Error, msg, memberName, file, line);
    }

    public static void Critical(string msg, [CallerMemberName] string memberName = "", [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        Print_Internal(Level.Critical, msg, memberName, file, line);
    }

    internal static unsafe delegate*<Level, NativeString, NativeString, NativeString, int, void> Print_Impl = null;
}
