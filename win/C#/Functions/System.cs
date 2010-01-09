/*  System.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using Microsoft.Win32;

namespace Handbrake.Functions
{
    class SystemInfo
    {
        /// <summary>
        /// Returns the total physical ram in a system
        /// </summary>
        /// <returns></returns>
        public static uint TotalPhysicalMemory
        {
            get
            {
                Win32.MEMORYSTATUS memStatus = new Win32.MEMORYSTATUS();
                Win32.GlobalMemoryStatus(ref memStatus);

                uint memoryInfo = memStatus.dwTotalPhys;
                memoryInfo = memoryInfo/1024/1024;

                return memoryInfo;
            }
        }

        /// <summary>
        /// Get the number of CPU Cores
        /// </summary>
        /// <returns>Object</returns>
        public static Object GetCpuCount
        {
            get
            {
                RegistryKey regKey = Registry.LocalMachine;
                regKey = regKey.OpenSubKey("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
                return regKey == null ? 0 : regKey.GetValue("ProcessorNameString");
            }
        }

        /// <summary>
        /// Get the System screen size information.
        /// </summary>
        /// <returns>System.Windows.Forms.Scree</returns>
        public static Screen ScreenBounds
        {
            get { return Screen.PrimaryScreen; }
        }
    }
}
