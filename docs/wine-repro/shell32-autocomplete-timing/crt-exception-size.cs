// Original, AI-assisted (Codex), LGPL-2.1-or-later.
using System;
using System.Runtime.InteropServices;

internal static class CrtExceptionSize
{
    private const uint SearchSystem32 = 0x800;

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
    private static extern IntPtr LoadLibraryExW(string module, IntPtr reserved, uint flags);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint QueryExceptionSize();

    private static void Measure(string name)
    {
        IntPtr module = LoadLibraryExW(name, IntPtr.Zero, SearchSystem32);
        if (module == IntPtr.Zero)
        {
            Console.WriteLine($"crt_query_size module={name} load_error={Marshal.GetLastWin32Error()}");
            return;
        }

        try
        {
            if (NativeLibrary.TryGetExport(module, "__CxxQueryExceptionSize", out var address))
            {
                var query = Marshal.GetDelegateForFunctionPointer<QueryExceptionSize>(address);
                Console.WriteLine($"crt_query_size module={name} bytes={query()}");
            }
            else
            {
                Console.WriteLine($"crt_query_size module={name} export=missing");
            }
        }
        finally
        {
            NativeLibrary.Free(module);
        }
    }

    public static int Main()
    {
        if (!OperatingSystem.IsWindows() || RuntimeInformation.ProcessArchitecture != Architecture.X64)
        {
            Console.WriteLine("crt_query_size=windows_x64_required");
            return 2;
        }

        Console.WriteLine($"probe=crt-exception-size arch=x64 dotnet={Environment.Version}");
        Measure("vcruntime140.dll");
        Measure("ucrtbase.dll");
        Measure("msvcrt.dll");
        return 0;
    }
}
