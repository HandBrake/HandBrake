// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeVersionHelper.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Version Utility
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Text;

    using HandBrake.Interop.Interop.HbLib;

    public class HandBrakeVersionHelper
    {
        /// <summary>
        /// Gets the HandBrake version string.
        /// </summary>
        public static string Version
        {
            get
            {
                var versionPtr = HBFunctions.hb_get_version(IntPtr.Zero); // Pointer isn't actually used.
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
                return HBFunctions.hb_get_build(IntPtr.Zero);
            }
        }

        public static string GetVersion()
        {
            return IsNightly() ? string.Format("Nightly {0} ({1})", Version, Build) : string.Format("{0} ({1})", Version, Build);
        }

        public static string GetVersionShort()
        {
            return string.Format("{0} {1}", Version, Build);
        }

        public static bool IsNightly()
        {
            // 01 = Unofficial Builds.  00 = Official Tagged Releases.
            return Build.ToString().EndsWith("01");
        }

        public static int? NightlyBuildAge()
        {
            // We consider any build older than 1 month, to be old.
            try
            {
                string build = Build.ToString().Substring(0, 8);
                DateTime buildDate;

                if (DateTime.TryParseExact(build, "yyyyMMdd", null, DateTimeStyles.None, out buildDate))
                {
                    TimeSpan days = DateTime.Now.Date - buildDate;
                    return days.Days;
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e);
            }

            return null;
        }
    }
}
