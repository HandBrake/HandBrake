// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VersionHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Version Utility
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Reflection;

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
            Version version = Assembly.GetEntryAssembly().GetName().Version;
            return IsNightly() ? string.Format("svn{0} (Nightly Build)", version.Revision) : string.Format("{0}.{1}.{2}.{3}", version.Major, version.Minor, version.Build, version.Revision);
        }

        /// <summary>
        /// The is nightly.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static bool IsNightly()
        {
            Version version = Assembly.GetEntryAssembly().GetName().Version;

            // The MSBuild.xml script sets 0.0.0 for nightly builds.
            return version.Major == 0 && version.Minor == 0 && version.Build == 0;
        }

        /// <summary>
        /// The get platform bitness.
        /// </summary>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static string GetPlatformBitnessVersion()
        {
            return System.Environment.Is64BitProcess ? "64bit Version" : "32bit Version";
        }

        /// <summary>
        /// Is a 64 bit app.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static bool Is64Bit()
        {
            return System.Environment.Is64BitProcess;
        }
    }
}
