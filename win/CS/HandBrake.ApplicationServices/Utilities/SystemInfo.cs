// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SystemInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The System Information.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System.Windows.Forms;
    using System.Management;
    using Microsoft.Win32;

    /// <summary>
    /// The System Information.
    /// </summary>
    public class SystemInfo
    {
        /// <summary>
        /// Gets the total physical ram in a system
        /// </summary>
        /// <returns>The total memory in the system</returns>
        public static ulong TotalPhysicalMemory
        {
            get
            {
                Win32.MEMORYSTATUSEX memStat = new Win32.MEMORYSTATUSEX { dwLength = 64 };
                Win32.GlobalMemoryStatusEx(ref memStat);

                ulong value = memStat.ullTotalPhys / 1024 / 1024;
                return value;
            }
        }

        /// <summary>
        /// Gets the number of CPU Cores
        /// </summary>
        /// <returns>Object</returns>
        public static object GetCpuCount
        {
            get
            {
                RegistryKey regKey = Registry.LocalMachine;
                regKey = regKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
                return regKey == null ? 0 : regKey.GetValue("ProcessorNameString");
            }
        }

        /// <summary>
        /// Gets the System screen size information.
        /// </summary>
        /// <returns>System.Windows.Forms.Scree</returns>
        public static Screen ScreenBounds
        {
            get { return Screen.PrimaryScreen; }
        }
        public static object GetGPUDriverVersion
        {
            get
            {
                ManagementObjectSearcher searcher = new ManagementObjectSearcher(
                "select * from " + "Win32_VideoController");
                foreach (ManagementObject share in searcher.Get())
                {
                    foreach (PropertyData PC in share.Properties)
                    {
                        if (PC.Name.Equals("DriverVersion"))
                            return PC.Value;
                    }
                }
                return null;
            }
        }
        public static object GetGPUName
        {
            get
            {
                ManagementObjectSearcher searcher = new ManagementObjectSearcher(
                "select * from " + "Win32_VideoController");
                foreach (ManagementObject share in searcher.Get())
                {
                    foreach (PropertyData PC in share.Properties)
                    {
                        if (PC.Name.Equals("Name"))
                            return PC.Value;
                    }
                }
                return null;
            }
        }
    }
}