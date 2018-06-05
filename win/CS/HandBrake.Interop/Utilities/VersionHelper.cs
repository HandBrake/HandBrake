// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VersionHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Version Utility
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Utilities
{
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces;

    /// <summary>
    /// Version Utility
    /// </summary>
    public class VersionHelper
    {
        /// <summary>
        /// The get build.
        /// </summary>
        /// <returns>
        /// The <see cref="int"/>.
        /// </returns>
        public static string GetVersion()
        {
            IHandBrakeInstance instance = HandBrakeInstanceManager.MasterInstance;
       
            return IsNightly() ? string.Format("Nightly {0} ({1})", instance.Version, instance.Build) : string.Format("{0} ({1})", instance.Version, instance.Build);
        }

        public static string GetVersionShort()
        {
            IHandBrakeInstance instance = HandBrakeInstanceManager.MasterInstance;
            return string.Format("{0} {1}", instance.Version, instance.Build);
        }

        /// <summary>
        /// The is nightly.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static bool IsNightly()
        {
            IHandBrakeInstance instance = HandBrakeInstanceManager.MasterInstance;

            // 01 = Unofficial Builds.  00 = Official Tagged Releases.
            return instance.Build.ToString().EndsWith("01");
        }
    }
}
