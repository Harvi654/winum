// Original, AI-assisted (Codex), LGPL-2.1-or-later.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;
using Microsoft.Diagnostics.Tracing;

internal static class EventPipeSummary
{
    private static readonly Regex TypeName = new Regex(
        @"\A(?:System|Microsoft|Autodesk)(?:[.+][A-Za-z_][A-Za-z_0-9]*){1,20}\z",
        RegexOptions.CultureInvariant);
    private static readonly Regex MethodLabel = new Regex(
        @"\A[A-Za-z_<>.][A-Za-z_0-9.<>=+`-]{0,239}\z", RegexOptions.CultureInvariant);
    private static readonly Regex ModuleLabel = new Regex(
        @"\A[A-Za-z_0-9.-]{1,160}\z", RegexOptions.CultureInvariant);

    private sealed record Entry(double Milliseconds, int Thread, string Type);
    private sealed record Method(long Module, ulong Start, string Type, string Name);

    private static void KeepRecent(Queue<Entry> history, Entry entry)
    {
        if (history.Count == 12) history.Dequeue();
        history.Enqueue(entry);
    }

    private static int Main(string[] args)
    {
        ulong returnPc = 0;
        if (args.Length < 2 || args.Length > 3 || !int.TryParse(args[1], NumberStyles.AllowHexSpecifier,
                                                                 CultureInfo.InvariantCulture, out int thread) ||
            (args.Length == 3 && (!ulong.TryParse(args[2], NumberStyles.AllowHexSpecifier,
                                                  CultureInfo.InvariantCulture, out returnPc) || returnPc == 0)))
            return 2;

        ulong callPc = returnPc != 0 ? returnPc - 1 : 0;
        CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
        var recent = new Queue<Entry>();
        var selected = new Queue<Entry>();
        var latest = 0.0;
        var total = 0;
        var matching = 0;
        var methodEvents = 0;
        var methods = new Dictionary<long, Method>();
        var modules = new Dictionary<long, string>();
        string status = "complete";

        try
        {
            using var source = new EventPipeEventSource(args[0]);
            if (callPc != 0)
            {
                source.Clr.MethodLoadVerbose += item =>
                {
                    ++methodEvents;
                    if (item.MethodSize <= 0 || callPc < item.MethodStartAddress ||
                        callPc - item.MethodStartAddress >= (ulong)item.MethodSize)
                        return;
                    var type = item.MethodNamespace;
                    var name = item.MethodName;
                    methods[item.MethodID] = new Method(item.ModuleID, item.MethodStartAddress,
                        MethodLabel.IsMatch(type) ? type : "<redacted>",
                        MethodLabel.IsMatch(name) ? name : "<redacted>");
                };
                source.Clr.MethodUnloadVerbose += item =>
                {
                    if (methods.TryGetValue(item.MethodID, out var method) && method.Start == item.MethodStartAddress)
                        methods.Remove(item.MethodID);
                };
                source.Clr.LoaderModuleLoad += item =>
                {
                    var path = item.ModuleILPath;
                    var basename = path[(Math.Max(path.LastIndexOf('/'), path.LastIndexOf('\\')) + 1)..];
                    if (ModuleLabel.IsMatch(basename)) modules[item.ModuleID] = basename;
                };
            }
            source.Clr.ExceptionStart += item =>
            {
                string name = item.ExceptionType;
                if (name.Length > 240 || !TypeName.IsMatch(name)) name = "<redacted>";
                var entry = new Entry(item.TimeStampRelativeMSec, item.ThreadID, name);
                ++total;
                latest = Math.Max(latest, entry.Milliseconds);
                KeepRecent(recent, entry);
                if (entry.Thread == thread)
                {
                    ++matching;
                    KeepRecent(selected, entry);
                }
            };
            source.Process();
        }
        catch (FormatException)
        {
            status = total > 0 || methodEvents > 0 ? "partial_format_error" : "format_error";
        }
        catch (EndOfStreamException)
        {
            status = total > 0 || methodEvents > 0 ? "partial_end_of_stream" : "end_of_stream";
        }
        catch (Exception error)
        {
            Console.WriteLine($"eventpipe_reader_error={error.GetType().Name}");
            return 2;
        }

        Console.WriteLine($"eventpipe_parse_status={status}");
        if (returnPc != 0)
        {
            Console.WriteLine($"eventpipe_method_events={methodEvents} matches={methods.Count}");
            if (methods.Count == 1)
            {
                foreach (var method in methods.Values)
                {
                    string module = modules.TryGetValue(method.Module, out var name) ? name : "<unknown>";
                    Console.WriteLine($"eventpipe_return_method={method.Type}.{method.Name} module={module} offset=0x{callPc - method.Start:x}");
                }
            }
        }
        Console.WriteLine($"exception_events={total} crash_thread=0x{thread:x4} on_crash_thread={matching}");
        if (total > 0)
            Console.WriteLine($"last_exception_ms={latest:F1}");
        foreach (var entry in matching > 0 ? selected : recent)
            Console.WriteLine($"exception_event ms={entry.Milliseconds:F1} tid=0x{entry.Thread:x4} type={entry.Type}");
        return 0;
    }
}
