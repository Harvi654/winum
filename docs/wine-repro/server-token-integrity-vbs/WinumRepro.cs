// Original, AI-assisted (Claude), LGPL-2.1-or-later.
// A minimal .NET Framework class for COM, registered per user only by
// per-user-dotnet-class.bat, the way per-user .NET add-ins register.
using System.Reflection;
using System.Runtime.InteropServices;

[assembly: AssemblyVersion("1.0.0.0")]
[assembly: ComVisible(false)]

namespace WinumRepro
{
    [ComVisible(true)]
    [Guid("6F3C2A51-9B7E-4C1D-8E2F-7A0B5D4C3E21")]
    [ProgId("WinumRepro.PerUserClass")]
    [ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class PerUserClass
    {
        public int Answer() { return 42; }
    }
}
