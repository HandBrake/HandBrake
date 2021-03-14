// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SystemInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The System Information.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Management;
    using System.Windows.Forms;

    using Microsoft.Win32;

    /// <summary>
    /// The System Information.
    /// </summary>
    public class SystemInfo
    {
        private static int cpuCoreCount = -1;

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

        public static object GetCpu
        {
            get
            {
                RegistryKey regKey = Registry.LocalMachine;
                regKey = regKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
                return regKey == null ? 0 : regKey.GetValue("ProcessorNameString");
            }
        }

        public static int GetCpuCoreCount
        {
            get
            {
                if (cpuCoreCount != -1)
                {
                    return cpuCoreCount;
                }

                int coreCount = 0;
                var cpuList = new System.Management.ManagementObjectSearcher("Select NumberOfCores from Win32_Processor").Get();

                foreach (var item in cpuList)
                {
                    coreCount += int.Parse(item["NumberOfCores"].ToString());
                }

                cpuCoreCount = coreCount;

                return cpuCoreCount;
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

      /// <summary>
        /// Gets the get gpu driver version.
        /// </summary>
        public static List<string> GetGPUInfo
        {
            get
            {
                List<string> gpuInfo = new List<string>();

                try
                {
                    ManagementObjectSearcher searcher =
                        new ManagementObjectSearcher("select DriverVersion, Name from " + "Win32_VideoController");

                    foreach (ManagementObject share in searcher.Get())
                    {
                        string gpu = string.Empty, version = string.Empty;

                        foreach (PropertyData pc in share.Properties)
                        {
                            if (!string.IsNullOrEmpty(pc.Name) && pc.Value != null)
                            {
                                if (pc.Name.Equals("DriverVersion")) version = pc.Value.ToString();
                                if (pc.Name.Equals("Name")) gpu = pc.Value.ToString();
                            }
                        }

                        if (string.IsNullOrEmpty(gpu))
                        {
                            gpu = "Unknown GPU";
                        }

                        if (string.IsNullOrEmpty(version))
                        {
                            version = "Unknown Driver Version";
                        }

                        gpuInfo.Add(string.Format("{0} - {1}", gpu, version));
                    }
                }
                catch (Exception)
                {
                    // Do Nothing. We couldn't get GPU Information.
                }

                return gpuInfo;
            }
        }

        public static bool IsWindows10()
        {
            OperatingSystem os = Environment.OSVersion;
            if (os.Version.Major >= 10)
            {
                return true;
            }

            return false;
        }

        public static bool IsAppsUsingDarkTheme()
        {
            object value = Registry.GetValue("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", "AppsUseLightTheme", null);
            if (value != null)
            {
                return (int)value != 1;
            }

            return false;
        }
    }
}
