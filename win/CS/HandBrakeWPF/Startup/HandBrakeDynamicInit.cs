// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeDynamic.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Dynamically load the correct hb.dll.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Startup
{
    using System.IO;
    using System.Runtime.InteropServices;

    using HandBrakeWPF.Utilities;

    public static class HandBrakeDynamic
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool SetDllDirectory(string lpPathName);

        public static void Init()
        {
            string dllPath = SystemInfo.IsArmDevice ? "arm64" : string.Empty;
            string dllDir = Path.Combine(Directory.GetCurrentDirectory(), dllPath);
            SetDllDirectory(dllDir);
        }
    }
}
