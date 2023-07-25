// --------------------------------------------------------------------------------------------------------------------
// <copyright file="GpuInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Information about the system GPU's
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System;

    public class GpuInfo
    {
        public GpuInfo(string name, string version)
        {
            this.Name = name;
            this.Version = version;

            if (string.IsNullOrEmpty(this.Name))
            {
                this.Name = "Unknown GPU";
            }
        }

        public string Name { get; private set; }

        public string Version { get; private set; }

        public string DisplayValue
        {
            get
            {
                string warning = string.Empty;
                if (this.IsIntel && !IsIntelDriverSupported)
                {
                    warning = string.Format(" - (Unsupported Driver Detected)");
                }

                return string.Format("{0} - {1}{2}", Name, Version, warning);
            }
        }

        public bool IsIntel
        {
            get
            {
                return Name.Contains("Intel", StringComparison.InvariantCultureIgnoreCase);
            }
        }

        public bool IsNvidia
        {
            get
            {
                return Name.Contains("Nvidia", StringComparison.InvariantCultureIgnoreCase);
            }
        }

        public bool IsIntelDriverSupported
        {
            get
            {
                if (this.IsIntel && !string.IsNullOrEmpty(Version))
                {
                    string[] versionSplit = this.Version.Split('.');
                    if (versionSplit.Length >= 1)
                    {
                        if (int.TryParse(versionSplit[0], out int majorVersion) && majorVersion >= 31)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }
        }
    }
}