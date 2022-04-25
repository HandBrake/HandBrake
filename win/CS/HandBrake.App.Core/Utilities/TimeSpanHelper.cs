// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TimeSpanHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper functions for handling <see cref="TimeSpan" /> structures
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Utilities
{
    using System;
    using System.Globalization;

    /// <summary>
    /// Helper functions for handling <see cref="TimeSpan"/> structures
    /// </summary>
    public static class TimeSpanHelper
    {
        /// <summary>
        /// Parses chapter time start value from a chapter marker input file. 
        /// </summary>
        /// <param name="chapterStartRaw">
        /// The raw string value parsed from the input file
        /// </param>
        /// <returns>
        /// The <see cref="TimeSpan"/>.
        /// </returns>
        public static TimeSpan ParseChapterTimeStart(string chapterStartRaw)
        {
            if (string.IsNullOrEmpty(chapterStartRaw))
            {
                return TimeSpan.MinValue;
            }

            // Format: 02:35:05 and 02:35:05.2957333
            TimeSpan converted;
            string fromTime = chapterStartRaw.Trim().TrimEnd('0');
            if (TimeSpan.TryParseExact(fromTime, new[] { @"HH\:mm\:ss", @"hh\:mm\:ss\.FFFFFFF", }, CultureInfo.InvariantCulture, out converted))
            {
                return converted;
            }

            return TimeSpan.Zero;
        }
    }
}
