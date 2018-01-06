// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISystemInfo.cs" company="HandBrake Project (http://handbrake.fr)">
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

    /// <summary>
    /// The System Information.
    /// </summary>
    public interface ISystemInfo
    {
        /// <summary>
        /// Gets the total physical ram in a system
        /// </summary>
        /// <returns>The total memory in the system</returns>
        ulong TotalPhysicalMemory { get; }

        /// <summary>
        /// Gets the number of CPU Cores
        /// </summary>
        /// <returns>Object</returns>
        string CPUInformation { get; }

        /// <summary>
        /// Gets the System screen size information.
        /// </summary>
        /// <returns>Screen Size</returns>
        Size ScreenBounds { get; }

        /// <summary>
        /// Gets the get gpu driver version.
        /// </summary>
        List<string> GPUInfo { get; }

        /// <summary>
        /// Gets the Install Location of the Application.
        /// </summary>
        string InstallLocation { get; }

        /// <summary>
        /// Gets the AppData Location.
        /// </summary>
        string AppDataLocation { get; }
    }
}