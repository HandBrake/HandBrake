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
    using System.Collections.Generic;
    using System.Drawing;
    using System.IO;
    using Windows.ApplicationModel;
    using Windows.Storage;

    public class SystemInfo : ISystemInfo
    {
        public ulong TotalPhysicalMemory => 52;

        public string CPUInformation => "Potato";

        public Size ScreenBounds => new Size(0, 0);

        public List<string> GPUInfo => new List<string>();

        public string InstallLocation => Package.Current.InstalledLocation.Path;

        public string AppDataLocation => Path.GetDirectoryName(ApplicationData.Current.LocalFolder.Path);

        public bool SupportsOpticalDrive => false;
    }
}