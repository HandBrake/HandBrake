// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CsvHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Utility functions for writing CSV files
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Utilities
{
    /// <summary>
    /// Utility functions for writing CSV files
    /// </summary>
    public sealed class CsvHelper
    {
        private const string QUOTE = "\"";
        private const string ESCAPED_QUOTE = "\"\"";
        private static readonly char[] CHARACTERS_THAT_MUST_BE_QUOTED = { ',', '"', '\n', '\t' };

        /// <summary>
        /// Properly escapes a string value containing reserved characters with double quotes "..." before it is written to a CSV file.
        /// </summary>
        /// <param name="value">Value to be escaped</param>
        /// <returns>Fully escaped value</returns>
        public static string Escape(string value)
        {
            if (value.Contains(QUOTE))
                value = value.Replace(QUOTE, ESCAPED_QUOTE);

            if (value.IndexOfAny(CHARACTERS_THAT_MUST_BE_QUOTED) > -1)
                value = QUOTE + value + QUOTE;

            return value;
        }
    }
}
