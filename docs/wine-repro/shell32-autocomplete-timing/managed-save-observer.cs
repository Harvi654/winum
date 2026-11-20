// Original, AI-assisted (Codex), LGPL-2.1-or-later.
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

#if !PROBE_SELF_TEST
using Autodesk.AutoCAD.Runtime;
#endif

public static class WinumSaveObserver
{
    private const int Limit = 512;
    private const int FrameLimit = 12;
    private const int NativeFrameLimit = 24;
    private const int TargetLimit = 24;
    private static readonly object LogGate = new();
    private static readonly Regex SymbolName = new(@"\A[A-Za-z0-9_.+`<>$-]{1,240}\z");
    private static StreamWriter output;
    private static string markerStem;
    private static int records;
    private static int targetRecords;
    private static bool selfTestSeen;
    [ThreadStatic]
    private static bool writing;

    [DllImport("kernel32.dll")]
    private static extern uint GetCurrentThreadId();

    [DllImport("ntdll.dll")]
    private static extern ushort RtlCaptureStackBackTrace(uint skip, uint count, [Out] IntPtr[] frames, IntPtr hash);

    [DllImport("kernel32.dll", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetModuleHandleExW(uint flags, IntPtr address, out IntPtr module);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    private static extern uint GetModuleFileNameW(IntPtr module, StringBuilder name, uint capacity);

    private static uint NativeThreadId() => OperatingSystem.IsWindows()
        ? GetCurrentThreadId() : (uint)Environment.CurrentManagedThreadId;

    private sealed class ProbeCheckException : System.Exception { }

    private static string Symbol(string value) =>
        value != null && SymbolName.IsMatch(value) ? value : "<redacted>";

    private static string SavePhase()
    {
        if (File.Exists(markerStem + ".save_command_exit"))
            return "after_save";
        if (File.Exists(markerStem + ".save_begin"))
            return "saving";
        if (File.Exists(markerStem + ".save_command_enter"))
            return "command";
        return "before_save";
    }

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

    private static void WriteNativeFrames()
    {
        if (!OperatingSystem.IsWindows())
            return;

        const uint FromAddress = 0x4;
        const uint UnchangedRefcount = 0x2;
        var addresses = new IntPtr[NativeFrameLimit];
        int count = RtlCaptureStackBackTrace(1, NativeFrameLimit, addresses, IntPtr.Zero);
        var path = new StringBuilder(1024);
        for (int i = 0; i < count; i++)
        {
            string name = "unresolved";
            string offset = "-";
            if (GetModuleHandleExW(FromAddress | UnchangedRefcount, addresses[i], out var module))
            {
                path.Clear();
                uint length = GetModuleFileNameW(module, path, (uint)path.Capacity);
                if (length > 0 && length < path.Capacity)
                {
                    name = Symbol(Path.GetFileName(path.ToString()).ToLowerInvariant());
                    offset = $"0x{addresses[i].ToInt64() - module.ToInt64():x}";
                }
            }
            output.WriteLine($"managed_native_frame index={i} module={name} offset={offset}");
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

        markerStem = Path.Combine(Path.GetDirectoryName(path), "open-events.txt");
        output = new StreamWriter(new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.Read))
        {
            AutoFlush = true,
        };
        output.WriteLine($"managed_probe=armed tid=0x{NativeThreadId():x}");
        output.WriteLine($"managed_probe=runtime version={Environment.Version}");
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
            bool target = args.Exception is NotImplementedException;
            if (target ? targetRecords >= TargetLimit : records >= Limit)
                return;
            string phase = target ? SavePhase() : null;
            string type = Symbol(args.Exception.GetType().FullName);
            string suffix = phase == null ? "" : $" phase={phase}";
            output.WriteLine($"managed_exception tid=0x{NativeThreadId():x} type={type} hr=0x{args.Exception.HResult:x8}{suffix}");
            if (++records == Limit)
                output.WriteLine("managed_probe=limit");
            if (!target)
                return;
            if (++targetRecords == TargetLimit)
                output.WriteLine("managed_probe=target_limit");
            WriteFrames(args.Exception);
            WriteNativeFrames();
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
        File.WriteAllText(markerStem + ".save_begin", "");
        try { StackCheck.Throw(); }
        catch (NotImplementedException) { }
    }
#endif
}
