// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CsvHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Utility functions for writing CSV files
// </summary>
// --------------------------------------------------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace HandBrake.App.Core.Utilities
{
    /// <summary>
    /// Utility functions for working with CSV files
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
            {
                value = value.Replace(QUOTE, ESCAPED_QUOTE);
            }

            if (value.IndexOfAny(CHARACTERS_THAT_MUST_BE_QUOTED) > -1)
            {
                value = QUOTE + value + QUOTE;
            }

            return value;
        }

        /// <summary>
        /// Reads a CSV or TSV file and parses it according to RFC 4180.
        /// </summary>
        /// <param name="filePath">Path to the csv or tsv file. </param>
        /// <returns>Parsed rows where each row is an array of column values.</returns>
        public static IList<string[]> ParseFile(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
            {
                throw new ArgumentException("File path must be provided.", nameof(filePath));
            }

            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException("The specified file was not found.", filePath);
            }

            var rows = new List<string[]>();
            var delimiter = DetermineDelimiter(filePath);

            using (var reader = new StreamReader(filePath, Encoding.UTF8, true))
            {
                var fieldBuilder = new StringBuilder();
                var currentRow = new List<string>();
                var inQuotes = false;

                while (reader.Peek() != -1)
                {
                    var character = (char)reader.Read();

                    if (inQuotes)
                    {
                        if (character == '"')
                        {
                            if (reader.Peek() == '"')
                            {
                                reader.Read();
                                fieldBuilder.Append('"');
                            }
                            else
                            {
                                inQuotes = false;
                            }
                        }
                        else
                        {
                            fieldBuilder.Append(character);
                        }

                        continue;
                    }

                    if (character == '"')
                    {
                        if (fieldBuilder.Length > 0)
                        {
                            throw new FormatException("There was a malformed quoted field detected.");
                        }

                        inQuotes = true;
                    }
                    else if (character == delimiter)
                    {
                        currentRow.Add(fieldBuilder.ToString());
                        fieldBuilder.Clear();
                    }
                    else if (character == '\r' || character == '\n')
                    {
                        if (character == '\r' && reader.Peek() == '\n')
                        {
                            reader.Read();
                        }

                        currentRow.Add(fieldBuilder.ToString());
                        fieldBuilder.Clear();
                        rows.Add(currentRow.ToArray());
                        currentRow.Clear();
                    }
                    else
                    {
                        fieldBuilder.Append(character);
                    }
                }

                if (inQuotes)
                {
                    throw new FormatException("There was a unterminated quoted field detected");
                }

                if (fieldBuilder.Length > 0 || currentRow.Count > 0)
                {
                    currentRow.Add(fieldBuilder.ToString());
                    rows.Add(currentRow.ToArray());
                }
            }

            return rows;
        }

        private static char DetermineDelimiter(string filePath)
        {
            var extension = Path.GetExtension(filePath);
            return extension.Equals(".tsv", StringComparison.OrdinalIgnoreCase) ? '\t' : ',';
        }
    }
}
