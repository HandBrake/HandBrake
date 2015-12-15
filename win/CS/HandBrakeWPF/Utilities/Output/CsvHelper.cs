using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HandBrakeWPF.Utilities.Output
{
    /// <summary>
    /// Utilitiy functions for writing CSV files
    /// </summary>
    internal sealed class CsvHelper
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
