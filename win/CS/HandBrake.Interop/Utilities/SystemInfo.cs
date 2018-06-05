// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SystemInfo.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The System Information.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Utilities
{
    using System;

    using Interop.HbLib;

    /// <summary>
    /// The System Information.
    /// </summary>
    public class SystemInfo
    {
        /// <summary>
        /// Gets a value indicating whether is qsv available.
        /// </summary>
        public static bool IsQsvAvailable
        {
            get
            {
                try
                {
                    return HBFunctions.hb_qsv_available() != 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether is qsv available.
        /// </summary>
        public static bool IsQsvAvailableH264
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_H264) != 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether is qsv available.
        /// </summary>
        public static bool IsQsvAvailableH265
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_H265) != 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static bool IsQsvAvailableH26510bit
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_H265_10BIT) != 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }
    }
}
