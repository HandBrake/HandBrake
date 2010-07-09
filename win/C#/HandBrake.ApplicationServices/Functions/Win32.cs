/*  win32.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
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
            /// <summary>
            /// Unknown
            /// </summary>
            public UInt32 dwLength;

            /// <summary>
            /// Memory Load
            /// </summary>
            public UInt32 dwMemoryLoad;

            /// <summary>
            /// Total Physical Memory
            /// </summary>
            public UInt32 dwTotalPhys; // Used

            /// <summary>
            /// Available Physical Memory
            /// </summary>
            public UInt32 dwAvailPhys;

            /// <summary>
            /// Total Page File
            /// </summary>
            public UInt32 dwTotalPageFile;

            /// <summary>
            /// Available Page File
            /// </summary>
            public UInt32 dwAvailPageFile;

            /// <summary>
            /// Total Virtual Memory
            /// </summary>
            public UInt32 dwTotalVirtual;

            /// <summary>
            /// Available Virtual Memory
            /// </summary>
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

        /// <summary>
        /// Generate a Console Ctrl Event
        /// </summary>
        /// <param name="sigevent">
        /// The sigevent.
        /// </param>
        /// <param name="dwProcessGroupId">
        /// The dw process group id.
        /// </param>
        /// <returns>
        /// Bool true is sucess
        /// </returns>
        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool GenerateConsoleCtrlEvent(ConsoleCtrlEvent sigevent, int dwProcessGroupId);

        /// <summary>
        /// Console Ctrl Event
        /// </summary>
        public enum ConsoleCtrlEvent
        {
            /// <summary>
            /// Ctrl - C
            /// </summary>
            CTRL_C = 0,

            /// <summary>
            /// Ctrl - Break
            /// </summary>
            CTRL_BREAK = 1,

            /// <summary>
            /// Ctrl - Close
            /// </summary>
            CTRL_CLOSE = 2,
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern EXECUTION_STATE SetThreadExecutionState(EXECUTION_STATE esFlags);

        [FlagsAttribute]
        public enum EXECUTION_STATE : uint
        {
            ES_SYSTEM_REQUIRED = 0x00000001,
            ES_CONTINUOUS = 0x80000000,
            ES_AWAYMODE_REQUIRED = 0x00000040
        }

        /// <summary>
        /// Prevent the system from sleeping
        /// </summary>
        public static void PreventSleep()
        {
            SetThreadExecutionState(EXECUTION_STATE.ES_CONTINUOUS | EXECUTION_STATE.ES_SYSTEM_REQUIRED | EXECUTION_STATE.ES_AWAYMODE_REQUIRED);
        }

        /// <summary>
        ///  Allow the system to sleep.
        /// </summary>
        public static void AllowSleep()
        {
            SetThreadExecutionState(EXECUTION_STATE.ES_CONTINUOUS);
        }
    }
}
