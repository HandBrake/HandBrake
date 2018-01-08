// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SystemInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The System Information.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Drawing;
    using System.Management;
    using System.Windows.Forms;

    using HandBrake.Utilities;
    using Microsoft.Win32;

    /// <summary>
    /// The System Information.
    /// </summary>
    public class SystemInfo : ISystemInfo
    {
        public ulong TotalPhysicalMemory
        {
            get
            {
                Win32.MEMORYSTATUSEX memStat = new Win32.MEMORYSTATUSEX { dwLength = 64 };
                Win32.GlobalMemoryStatusEx(ref memStat);

                ulong value = memStat.ullTotalPhys / 1024 / 1024;
                return value;
            }
        }

        public string CPUInformation
        {
            get
            {
                RegistryKey regKey = Registry.LocalMachine;
                regKey = regKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
                return regKey == null ? string.Empty : regKey.GetValue("ProcessorNameString")?.ToString();
            }
        }

        public List<string> GPUInfo
        {
            get
            {
                List<string> gpuInfo = new List<string>();

                try
                {
                    ManagementObjectSearcher searcher =
                        new ManagementObjectSearcher("select * from " + "Win32_VideoController");

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

        public Size ScreenBounds
        {
            get
            {
                var screen = Screen.PrimaryScreen;
                return new Size(screen.Bounds.Width, screen.Bounds.Height);
            }
        }

        public string InstallLocation => Application.StartupPath;

        public string AppDataLocation => Application.UserAppDataPath;
    }
}