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

    private sealed record Entry(double Milliseconds, int Thread, string Type);

    private static void KeepRecent(Queue<Entry> history, Entry entry)
    {
        if (history.Count == 12) history.Dequeue();
        history.Enqueue(entry);
    }

    private static int Main(string[] args)
    {
        if (args.Length != 2 || !int.TryParse(args[1], NumberStyles.AllowHexSpecifier,
                                               CultureInfo.InvariantCulture, out int thread))
            return 2;

        CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
        var recent = new Queue<Entry>();
        var selected = new Queue<Entry>();
        var latest = 0.0;
        var total = 0;
        var matching = 0;
        string status = "complete";

        try
        {
            using var source = new EventPipeEventSource(args[0]);
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
            status = total > 0 ? "partial_format_error" : "format_error";
        }
        catch (EndOfStreamException)
        {
            status = total > 0 ? "partial_end_of_stream" : "end_of_stream";
        }
        catch (Exception error)
        {
            Console.WriteLine($"eventpipe_reader_error={error.GetType().Name}");
            return 2;
        }

        Console.WriteLine($"eventpipe_parse_status={status}");
        Console.WriteLine($"exception_events={total} crash_thread=0x{thread:x4} on_crash_thread={matching}");
        if (total > 0)
            Console.WriteLine($"last_exception_ms={latest:F1}");
        foreach (var entry in matching > 0 ? selected : recent)
            Console.WriteLine($"exception_event ms={entry.Milliseconds:F1} tid=0x{entry.Thread:x4} type={entry.Type}");
        return 0;
    }
}
