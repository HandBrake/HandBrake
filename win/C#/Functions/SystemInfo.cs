using System;
using System.Runtime.InteropServices;
using Microsoft.Win32;

namespace Handbrake.Functions
{
    class SystemInfo
    {
        #region CheckRam
        private struct MEMORYSTATUS
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
        private static extern void GlobalMemoryStatus
        (
            ref MEMORYSTATUS lpBuffer
        );


        /// <summary>
        /// Returns the total physical ram in a system
        /// </summary>
        /// <returns></returns>
        public uint TotalPhysicalMemory()
        {
            MEMORYSTATUS memStatus = new MEMORYSTATUS();
            GlobalMemoryStatus(ref memStatus);

            uint MemoryInfo = memStatus.dwTotalPhys;
            MemoryInfo = MemoryInfo / 1024 / 1024; 

            return MemoryInfo;
        }
        #endregion

        public Object getCpuCount()
        {
            RegistryKey RegKey = Registry.LocalMachine;
            RegKey = RegKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
            return RegKey.GetValue("ProcessorNameString");
        }

        public System.Windows.Forms.Screen screenBounds()
        {
            return System.Windows.Forms.Screen.PrimaryScreen;
        }
    }
}
