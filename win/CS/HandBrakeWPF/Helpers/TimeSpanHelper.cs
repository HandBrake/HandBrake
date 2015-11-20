using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HandBrakeWPF.Helpers
{
    using System.Globalization;

    /// <summary>
    /// Helper functions for handling <see cref="TimeSpan"/> structures
    /// </summary>
    internal static class TimeSpanHelper
    {
        /// <summary>
        /// Parses chapter time start value from a chapter marker input file. 
        /// </summary>
        /// <param name="chapterStartRaw">The raw string value parsed from the input file</param>
        internal static TimeSpan ParseChapterTimeStart(string chapterStartRaw)
        {
            //Format: 02:35:05 and 02:35:05.2957333
            return TimeSpan.ParseExact(chapterStartRaw,
                                        new[]
                                        {
                                                    @"hh\:mm\:ss",  // Handle whole seconds
                                                    @"hh\:mm\:ss\.FFFFFFF"  // Handle fraction seconds
                                        }, CultureInfo.InvariantCulture);
        }
    }
}
