﻿// --------------------------------------------------------------------------------------------------------------------
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

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.Interfaces;

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
            IHandBrakeInstance instance = HandBrakeInstanceManager.GetScanInstance(1);
       
            return IsNightly() ? string.Format("Nightly {0} ({1})", instance.Version, instance.Build) : string.Format("{0} ({1})", instance.Version, instance.Build);
        }

        /// <summary>
        /// The is nightly.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static bool IsNightly()
        {
            IHandBrakeInstance instance = HandBrakeInstanceManager.GetScanInstance(1);

            // 01 = Unofficial Builds.  00 = Official Tagged Releases.
            return instance.Build.ToString().EndsWith("01");
        }

        /// <summary>
        /// The get platform bitness.
        /// </summary>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static string GetPlatformBitnessVersion()
        {
            return System.Environment.Is64BitProcess ? "64bit" : "32bit";
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
