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
    using System;
    using System.Runtime.InteropServices;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Providers;
    using HandBrake.Interop.Interop.Providers.Interfaces;

    public class VersionHelper
    {
        private static IHbFunctions hbFunctions;

        static VersionHelper()
        {
            IHbFunctionsProvider hbFunctionsProvider = new HbFunctionsProvider();
            hbFunctions = hbFunctionsProvider.GetHbFunctionsWrapper();
        }

        /// <summary>
        /// The get build.
        /// </summary>
        /// <returns>
        /// The <see cref="int"/>.
        /// </returns>
        public static string GetVersion()
        {
            return IsNightly() ? string.Format("Nightly {0} ({1})", Version, Build) : string.Format("{0} ({1})", Version, Build);
        }

        public static string GetVersionShort()
        {
            return string.Format("{0} {1}", Version, Build);
        }

        /// <summary>
        /// The is nightly.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static bool IsNightly()
        {
            // 01 = Unofficial Builds.  00 = Official Tagged Releases.
            return Build.ToString().EndsWith("01");
        }

        /// <summary>
        /// Gets the HandBrake version string.
        /// </summary>
        public static string Version
        {
            get
            {
                var versionPtr = hbFunctions.hb_get_version(IntPtr.Zero); // Pointer isn't actually used.
                return Marshal.PtrToStringAnsi(versionPtr);
            }
        }

        /// <summary>
        /// Gets the HandBrake build number.
        /// </summary>
        public static int Build
        {
            get
            {
                return hbFunctions.hb_get_build(IntPtr.Zero);
            }
        }
    }
}
