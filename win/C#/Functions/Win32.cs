/*  win32.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */


namespace Handbrake.Functions
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Win32 API calls
    /// </summary>
    public class Win32
    {
        /// <summary>
        /// Set the Forground Window
        /// </summary>
        /// <param name="hWnd">
        /// The h wnd.
        /// </param>
        /// <returns>
        /// A Boolean true when complete.
        /// </returns>
        [DllImport("user32.dll")]
        public static extern bool SetForegroundWindow(int hWnd);

        /// <summary>
        /// Lock the workstation
        /// </summary>
        [DllImport("user32.dll")]
        public static extern void LockWorkStation();

        /// <summary>
        /// Exit Windows
        /// </summary>
        /// <param name="uFlags">
        /// The u flags.
        /// </param>
        /// <param name="dwReason">
        /// The dw reason.
        /// </param>
        /// <returns>
        /// an integer
        /// </returns>
        [DllImport("user32.dll")]
        public static extern int ExitWindowsEx(int uFlags, int dwReason);

        /// <summary>
        /// System Memory Status
        /// </summary>
        public struct MEMORYSTATUS // Unused var's are required here.
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

        /// <summary>
        /// Global Memory Status
        /// </summary>
        /// <param name="lpBuffer">
        /// The lp buffer.
        /// </param>
        [DllImport("kernel32.dll")]
        public static extern void GlobalMemoryStatus
        (
            ref MEMORYSTATUS lpBuffer
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool GenerateConsoleCtrlEvent(ConsoleCtrlEvent sigevent, int dwProcessGroupId);

        /// <summary>
        /// Console Ctrl Event
        /// </summary>
        public enum ConsoleCtrlEvent
        {
            CTRL_C = 0,
            CTRL_BREAK = 1,
            CTRL_CLOSE = 2,
        }
    }
}
