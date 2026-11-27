using System;
using System.Collections;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;

internal static class StartupHook
{
    private sealed class SampleState
    {
        public int Pending;
        public string Last = "";
    }

    private static readonly ConditionalWeakTable<object, SampleState> States = new();
    private static readonly object LogLock = new();
    private static string LogPath = "";

    public static void Initialize()
    {
        if (!string.Equals(Path.GetFileName(Environment.ProcessPath), "Inventor.exe",
                           StringComparison.OrdinalIgnoreCase))
            return;

        LogPath = Environment.GetEnvironmentVariable("INVENTOR_WPF_DETAILS_LOG") ?? "";
        if (LogPath.Length == 0)
            return;

        Log($"started pid={Environment.ProcessId}");
        new Thread(Watch) { IsBackground = true }.Start();
    }

    private static void Watch()
    {
        Assembly? wpf = null;
        while (wpf == null)
        {
            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
                if (assembly.GetName().Name == "PresentationCore")
                    wpf = assembly;
            if (wpf == null)
                Thread.Sleep(750);
        }

        try
        {
            Type helper = wpf.GetType("System.Windows.Media.VisualTreeHelper", true)!;
            MethodInfo getChild = helper.GetMethod("GetChild")!;
            MethodInfo count = helper.GetMethod("GetChildrenCount")!;
            PropertyInfo sources = wpf.GetType("System.Windows.PresentationSource", true)!
                                      .GetProperty("CurrentSources")!;
            Log("presentation_core=loaded");

            string lastError = "";
            while (true)
            {
                try
                {
                    foreach (object source in (IEnumerable)sources.GetValue(null)!)
                        Schedule(source, getChild, count);
                }
                catch (Exception error)
                {
                    string message = ErrorMessage(error);
                    if (lastError != message)
                        Log(message);
                    lastError = message;
                }
                Thread.Sleep(750);
            }
        }
        catch (Exception error)
        {
            Log(ErrorMessage(error));
        }
    }

    private static void Schedule(object source, MethodInfo getChild, MethodInfo count)
    {
        SampleState state = States.GetValue(source, static _ => new SampleState());
        if (Interlocked.Exchange(ref state.Pending, 1) != 0)
            return;

        try
        {
            object dispatcher = Read(source, "Dispatcher")!;
            Action sample = () =>
            {
                try { Sample(source, state, getChild, count); }
                catch (Exception error) { Report(state, ErrorMessage(error)); }
                finally { Volatile.Write(ref state.Pending, 0); }
            };
            dispatcher.GetType().GetMethod("InvokeAsync", new[] { typeof(Action) })!
                      .Invoke(dispatcher, new object[] { sample });
        }
        catch
        {
            Volatile.Write(ref state.Pending, 0);
            throw;
        }
    }

    private static void Sample(object source, SampleState state, MethodInfo getChild, MethodInfo count)
    {
        object? root = Read(source, "RootVisual");
        if (root == null || (root.GetType().Name != "FileDialogUI" &&
                            !Equals(Read(root, "Name"), "FileDialgWnd") &&
                            !Equals(Read(root, "Title"), "Open")))
            return;

        object? tab = null;
        object? properties = null;
        FindElements(root, getChild, count, ref tab, ref properties);
        string result = $"details_selected={Read(tab, "IsSelected") ?? "missing"}";
        if (properties == null)
        {
            Report(state, result + " file_properties=missing");
            return;
        }

        string? text = Read(properties, "Text") as string;
        string height = Convert.ToString(Read(properties, "ActualHeight"), CultureInfo.InvariantCulture) ?? "";
        Report(state, result + $" file_properties=present text_length={text?.Length ?? -1}" +
               $" height={height} visible={Read(properties, "IsVisible")}" +
               $" part_number_label={text?.Contains("Part Number:", StringComparison.OrdinalIgnoreCase) ?? false}");
    }

    private static void FindElements(object element, MethodInfo getChild, MethodInfo count,
                                     ref object? tab, ref object? properties)
    {
        switch (Read(element, "Name"))
        {
            case "DetailsTab": tab = element; break;
            case "FileProperties": properties = element; break;
        }

        int children = (int)count.Invoke(null, new[] { element })!;
        for (int index = 0; index < children && (tab == null || properties == null); index++)
        {
            var child = getChild.Invoke(null, new object[] { element, index })!;
            FindElements(child, getChild, count, ref tab, ref properties);
        }
    }

    private static object? Read(object? instance, string name) =>
        instance?.GetType().GetProperty(name)?.GetValue(instance);

    private static string ErrorMessage(Exception error) =>
        $"probe_error={(error.InnerException ?? error).GetType().Name}";

    private static void Report(SampleState state, string value)
    {
        if (state.Last != value)
            Log(value);
        state.Last = value;
    }

    private static void Log(string value)
    {
        lock (LogLock)
        {
            try { File.AppendAllText(LogPath, $"{DateTimeOffset.Now:HH:mm:ss.fff} {value}{Environment.NewLine}"); }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }
    }
}
