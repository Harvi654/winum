/* Original, AI-assisted (Codex), LGPL-2.1-or-later. */
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
#if WPF
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
#endif

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        if (args.Length != 1) return 2;
        Console.WriteLine($"probe_runtime={RuntimeInformation.FrameworkDescription}");
        Console.WriteLine("probe_stage=started");
        using var watchdog = new Timer(_ =>
        {
            Console.WriteLine("probe_stage=timeout");
            Environment.Exit(124);
        }, null, TimeSpan.FromSeconds(25), Timeout.InfiniteTimeSpan);

        try
        {
#if WPF
            return RunWpf(args[0]);
#else
            ReadDrawing(args[0]);
            Console.WriteLine("probe_stage=complete");
            return 0;
#endif
        }
        catch (Exception error)
        {
            ReportError(error);
            return 1;
        }
    }

    private static void ReportError(Exception error)
    {
        Console.WriteLine($"probe_exception={error.GetType().FullName} hresult=0x{error.HResult:x8}");
    }

    private static void ReadDrawing(string path)
    {
        try
        {
            using var missing = File.OpenRead(path + ".missing");
            throw new InvalidOperationException();
        }
        catch (FileNotFoundException)
        {
            Console.WriteLine("probe_stage=missing_file_caught");
        }

        byte[] bytes = File.ReadAllBytes(path);
        if (!bytes.AsSpan().StartsWith("AC1032"u8)) throw new InvalidDataException();
        Console.WriteLine($"probe_dwg_bytes={bytes.Length}");
        Console.WriteLine("probe_stage=file_read");
    }

#if WPF
    private static int RunWpf(string path)
    {
        var app = new Application { ShutdownMode = ShutdownMode.OnExplicitShutdown };
        app.DispatcherUnhandledException += (_, error) =>
        {
            ReportError(error.Exception);
            error.Handled = true;
            app.Shutdown(1);
        };

        var window = new Window
        {
            Title = "winum WPF probe",
            Width = 360,
            Height = 120,
            Content = new TextBlock { Text = "Opening the test file" }
        };
        window.ContentRendered += (_, _) =>
        {
            Console.WriteLine("probe_stage=main_window_rendered");
            ReadDrawing(path);
            var document = new Window
            {
                Owner = window,
                Title = "winum test document",
                Width = 300,
                Height = 100,
                Content = new TextBlock { Text = "File read completed" }
            };
            document.ContentRendered += (_, _) =>
            {
                Console.WriteLine("probe_stage=document_window_rendered");
                document.Dispatcher.BeginInvoke(DispatcherPriority.ApplicationIdle, new Action(() =>
                {
                    document.Close();
                    Console.WriteLine("probe_stage=complete");
                    app.Shutdown(0);
                }));
            };
            document.Show();
        };
        return app.Run(window);
    }
#endif
}
