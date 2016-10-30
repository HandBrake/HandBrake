// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TimeSpanHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper functions for handling <see cref="TimeSpan" /> structures
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Globalization;
    using System;

    /// <summary>
    /// Helper functions for handling <see cref="TimeSpan"/> structures
    /// </summary>
    internal static class TimeSpanHelper
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
        internal static TimeSpan ParseChapterTimeStart(string chapterStartRaw)
        {
            // Format: 02:35:05 and 02:35:05.2957333
            return TimeSpan.ParseExact(
                chapterStartRaw,
                new[] { @"hh\:mm\:ss", @"hh\:mm\:ss\.FFFFFFF" }, CultureInfo.InvariantCulture);  // Handle whole seconds then Handle fraction seconds
        }
    }
}
