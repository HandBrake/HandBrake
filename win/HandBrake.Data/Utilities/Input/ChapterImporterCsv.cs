// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChapterImporterCsv.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Handles the importing of Chapter information from CSV files
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities.Input
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using CsvHelper;
    using CsvHelper.Configuration;
    using HandBrake.Properties;
    using HandBrakeWPF.Exceptions;

    /// <summary>
    /// Handles the importing of Chapter information from CSV files
    /// </summary>
    public class ChapterImporterCsv
    {
        /// <summary>
        /// The file filter value for the OpenFileDialog
        /// </summary>
        public static string FileFilter => "CSV files (*.csv;*.tsv)|*.csv;*.tsv";

        /// <summary>
        /// Imports all chapter information from the given <see cref="filename"/> into the <see cref="chapterMap"/> dictionary.
        /// </summary>
        /// <param name="filename">
        /// The full path and filename of the chapter marker file to import
        /// </param>
        /// <param name="importedChapters">
        /// The imported Chapters.
        /// </param>
        public static void Import(string filename, ref Dictionary<int, Tuple<string, TimeSpan>> importedChapters)
        {
            // Determine the Delimeter Type for the Separate Values File Format.
            var extension = Path.GetExtension(filename);
            var delimeter = extension == ".tsv" ? "\t" : ",";

            using (var raw = File.OpenText(filename))
            {
                using (var csv = new CsvReader(raw, new Configuration
                {
                    TrimOptions = TrimOptions.Trim, // Remove excess whitespace from ends of imported values
                    QuoteAllFields = true, // Assume that our data will be properly escaped
                    Delimiter = delimeter,
                }))
                {
                    while (!csv.Read())
                    {
                        var rowNumber = csv.Context.Row;

                        try
                        {
                            // Only read the first two columns, anything else will be ignored but will not raise an error
                            var record = csv.Context.Record;

                            // null condition happens if the file is somehow corrupt during reading
                            if (record == null || record.Length < 2)
                            {
                                throw new Exception(Resources.ChaptersViewModel_UnableToImportChaptersLineDoesNotHaveAtLeastTwoColumns);
                            }

                            var rawchapterNumber = record.GetValue(0)?.ToString();
                            if (!int.TryParse(rawchapterNumber, out int chapterNumber))
                            {
                                throw new Exception(Resources.ChaptersViewModel_UnableToImportChaptersFirstColumnMustContainOnlyIntegerNumber);
                            }

                            // Store the chapter name at the correct index
                            importedChapters[chapterNumber] = new Tuple<string, TimeSpan>(record.GetValue(1)?.ToString().Trim(), TimeSpan.Zero);
                        }
                        catch (Exception exception)
                        {
                            throw new GeneralApplicationException(
                                Resources.ChaptersViewModel_UnableToImportChaptersWarning,
                                string.Format(Resources.ChaptersViewModel_UnableToImportChaptersMalformedLineMsg, rowNumber),
                                exception);
                        }
                    }
                }
            }
        }
    }
}