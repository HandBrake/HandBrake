// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Win32.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Win32 API calls
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.Runtime.InteropServices;
    using System.Windows.Threading;

    /// <summary>
    /// Win32 API calls
    /// </summary>
    public class Win32
    {
        /// <summary>
        /// Used to force UI Thread.
        /// </summary>
        private static Action<Action> executor = action => action();

        /// <summary>
        /// Set the Foreground Window
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
        /// Memory Status EX Struct
        /// </summary>
        public struct MEMORYSTATUSEX
        {
            public int dwLength;

            public int dwMemoryLoad;

            public ulong ullTotalPhys;

            public ulong ullAvailPhys;

            public ulong ullTotalPageFile;

            public ulong ullAvailPageFile;

            public ulong ullTotalVirtual;

            public ulong ullAvailVirtual;

            public ulong ullAvailExtendedVirtual;
        }

        /// <summary>
        /// Get the System Memory information
        /// </summary>
        /// <param name="lpBuffer">
        /// The lp buffer.
        /// </param>
        /// <returns>
        /// A boolean.
        /// </returns>
        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool GlobalMemoryStatusEx(ref MEMORYSTATUSEX lpBuffer);


        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern EXECUTION_STATE SetThreadExecutionState(EXECUTION_STATE esFlags);

        /// <summary>
        /// Execution State
        /// </summary>
        [Flags]
        public enum EXECUTION_STATE : uint
        {
            ES_SYSTEM_REQUIRED = 0x00000001,

            ES_CONTINUOUS = 0x80000000,
        }

        /// <summary>
        /// Initializes static members of the <see cref="Win32"/> class.
        /// </summary>
        static Win32()
        {
            InitializeWithDispatcher();
        }

        /// <summary>
        /// Prevent the system from sleeping
        /// </summary>
        public static void PreventSleep()
        {
            executor(() => SetThreadExecutionState(EXECUTION_STATE.ES_CONTINUOUS | EXECUTION_STATE.ES_SYSTEM_REQUIRED));
        }

        /// <summary>
        ///  Allow the system to sleep.
        /// </summary>
        public static void AllowSleep()
        {
            executor(() => SetThreadExecutionState(EXECUTION_STATE.ES_CONTINUOUS));
        }

        /// <summary>
        ///   Initializes the framework using the current dispatcher.
        /// </summary>
        public static void InitializeWithDispatcher()
        {
            var dispatcher = Dispatcher.CurrentDispatcher;

            SetUIThreadMarshaller(action =>
            {
                if (dispatcher.CheckAccess())
                    action();
                else dispatcher.Invoke(action);
            });
        }

        /// <summary>
        /// Sets a custom UI thread marshaller.
        /// </summary>
        /// <param name="marshaller">The marshaller.</param>
        public static void SetUIThreadMarshaller(Action<Action> marshaller)
        {
            executor = marshaller;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class PowerState
        {
            public ACLineStatus ACLineStatus;
            public BatteryFlag BatteryFlag;
            public Byte BatteryLifePercent;
            public Byte SystemStatusFlag;
            public Int32 BatteryLifeTime;
            public Int32 BatteryFullLifeTime;

            public static PowerState GetPowerState()
            {
                PowerState state = new PowerState();
                if (GetSystemPowerStatusRef(state))
                {
                    return state;
                }

                return null;
            }

            [DllImport("Kernel32", EntryPoint = "GetSystemPowerStatus")]
            private static extern bool GetSystemPowerStatusRef(PowerState sps);
        }

        public enum ACLineStatus : byte
        {
            Offline = 0,
            Online = 1,
            Unknown = 255
        }

        public enum BatteryFlag : byte
        {
            High = 1,
            Low = 2,
            Critical = 4,
            Charging = 8,
            NoSystemBattery = 128,
            Unknown = 255
        }

        [DllImport("Powrprof.dll", CharSet = CharSet.Auto, ExactSpelling = true)]
        public static extern bool SetSuspendState(bool hiberate, bool forceCritical, bool disableWakeEvent);


        [DllImport("dwmapi.dll")]
        private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int attrValue, int attrSize);

        public static void SetDarkTheme(IntPtr handle)
        {
            int mode = 1;
            if (DwmSetWindowAttribute(handle, 19, ref mode, sizeof(int)) != 0)
            {
                DwmSetWindowAttribute(handle, 20, ref mode, sizeof(int));
            }
        }
    }
}

