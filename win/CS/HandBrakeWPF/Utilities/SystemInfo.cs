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
    using System.Globalization;
    using System.Management;
    using System.Runtime.InteropServices;

    using HandBrakeWPF.Model;

    using Microsoft.Win32;

    /// <summary>
    /// The System Information.
    /// </summary>
    public class SystemInfo
    {
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

        public static bool IsArmDevice => RuntimeInformation.ProcessArchitecture == Architecture.Arm64;

        public static int GetCpuLogicalCount
        {
            get => Environment.ProcessorCount;
        }

        public static int MaximumSimultaneousInstancesSupported
        {
            get => Math.Min((int)Math.Round((decimal)SystemInfo.GetCpuLogicalCount / 2, 0), 12);
        }

        public static string ScreenBounds
        {
            get
            {
                string screenWidth = System.Windows.SystemParameters.PrimaryScreenWidth.ToString(CultureInfo.InvariantCulture);
                string screenHeight = System.Windows.SystemParameters.PrimaryScreenHeight.ToString(CultureInfo.InvariantCulture);

                return string.Format("{0}x{1}", screenWidth, screenHeight);
            }
        }

        public static List<GpuInfo> GetGPUInfo
        {
            get
            {
                List<GpuInfo> gpuInfo = new List<GpuInfo>();

                if (IsArmDevice)
                {
                    // We don't have .NET Framework on ARM64 devices so cannot use System.Management
                    // Default to ARM Chipset for now.
                    gpuInfo.Add(new GpuInfo("Arm Chipset", string.Empty));

                    return gpuInfo;
                }

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

                        gpuInfo.Add(new GpuInfo(gpu, version));
                    }
                }
                catch (Exception)
                {
                    // Do Nothing. We couldn't get GPU Information.
                }

                return gpuInfo;
            }
        }
        
        public static bool IsWindows10OrLater()
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
