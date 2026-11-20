// Original, AI-assisted (Codex), LGPL-2.1-or-later.
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;

#if !PROBE_SELF_TEST
using Autodesk.AutoCAD.Runtime;
#endif

public static class WinumSaveObserver
{
    private const int Limit = 512;
    private const int FrameLimit = 12;
    private static readonly object LogGate = new();
    private static readonly Regex SymbolName = new(@"\A[A-Za-z0-9_.+`<>$-]{1,240}\z");
    private static StreamWriter output;
    private static int records;
    private static bool selfTestSeen;
    [ThreadStatic]
    private static bool writing;

    [DllImport("kernel32.dll")]
    private static extern uint GetCurrentThreadId();

    private static uint NativeThreadId() => OperatingSystem.IsWindows()
        ? GetCurrentThreadId() : (uint)Environment.CurrentManagedThreadId;

    private sealed class ProbeCheckException : System.Exception { }

    private static string Symbol(string value) =>
        value != null && SymbolName.IsMatch(value) ? value : "<redacted>";

    private static void WriteFrames(System.Exception exception)
    {
        var stack = new StackTrace(exception, fNeedFileInfo: false);
        string source = "exception";
        if (stack.FrameCount == 0)
        {
            stack = new StackTrace(skipFrames: 1, fNeedFileInfo: false);
            source = "live";
        }

        int written = 0;
        for (int i = 0; i < stack.FrameCount && written < FrameLimit; i++)
        {
            var method = stack.GetFrame(i)?.GetMethod();
            if (method == null || method.DeclaringType == typeof(WinumSaveObserver))
                continue;
            string assembly = Symbol(method.Module.Assembly.GetName().Name);
            string name = Symbol(method.DeclaringType?.FullName + "." + method.Name);
            output.WriteLine($"managed_frame source={source} index={written++} assembly={assembly} method={name}");
        }
    }

#if !PROBE_SELF_TEST
    [CommandMethod("WINUMEXCEPTIONS")]
#endif
    public static void Arm()
    {
        string path = Environment.GetEnvironmentVariable("WINUM_ACAD_EXCEPTION_LOG");
        if (output != null || string.IsNullOrEmpty(path))
            return;

        output = new StreamWriter(new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
        {
            AutoFlush = true,
        };
        output.WriteLine($"managed_probe=armed tid=0x{NativeThreadId():x}");
        AppDomain.CurrentDomain.FirstChanceException += OnFirstChance;
        try
        {
            throw new ProbeCheckException();
        }
        catch (ProbeCheckException)
        {
            lock (LogGate)
                output.WriteLine($"managed_probe=self_test ok={(selfTestSeen ? 1 : 0)}");
        }
    }

    private static void OnFirstChance(object sender, FirstChanceExceptionEventArgs args)
    {
        if (args.Exception is ProbeCheckException)
        {
            selfTestSeen = true;
            return;
        }
        if (writing || !Monitor.TryEnter(LogGate))
            return;

        writing = true;
        try
        {
            if (records >= Limit)
                return;
            string type = Symbol(args.Exception.GetType().FullName);
            output.WriteLine($"managed_exception tid=0x{NativeThreadId():x} type={type} hr=0x{args.Exception.HResult:x8}");
            if (++records == Limit)
                output.WriteLine("managed_probe=limit");
            if (args.Exception is NotImplementedException)
                WriteFrames(args.Exception);
        }
        catch (System.Exception)
        {
            // Keep diagnostic I/O failures off AutoCAD's exception path.
        }
        finally
        {
            writing = false;
            Monitor.Exit(LogGate);
        }
    }

#if PROBE_SELF_TEST
    private static class StackCheck
    {
        [MethodImpl(MethodImplOptions.NoInlining)]
        public static void Throw() => throw new NotImplementedException("private test message");
    }

    public static void Main()
    {
        Arm();
        try { throw new InvalidOperationException("private test message"); }
        catch (InvalidOperationException) { }
        try { StackCheck.Throw(); }
        catch (NotImplementedException) { }
    }
#endif
}
