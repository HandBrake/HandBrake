using System;
using System.Runtime.InteropServices;

namespace Handbrake.Functions
{
    class Win32
    {
        [DllImport("user32.dll")]
        public static extern int SetForegroundWindow(int hWnd);

        [DllImport("user32.dll")]
        public static extern void LockWorkStation();

        [DllImport("user32.dll")]
        public static extern int ExitWindowsEx(int uFlags, int dwReason);

        public struct MEMORYSTATUS // Unused var's are requred here.
        {
            public UInt32 dwLength;
            public UInt32 dwMemoryLoad;
            public UInt32 dwTotalPhys; // Used
            public UInt32 dwAvailPhys;
            public UInt32 dwTotalPageFile;
            public UInt32 dwAvailPageFile;
            public UInt32 dwTotalVirtual;
            public UInt32 dwAvailVirtual;
        }

        [DllImport("kernel32.dll")]
        public static extern void GlobalMemoryStatus
        (
            ref MEMORYSTATUS lpBuffer
        );
    }
}
