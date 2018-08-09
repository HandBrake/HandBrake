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

    using HandBrakeWPF.Exceptions;
    using HandBrakeWPF.Properties;

    /// <summary>
    /// Handles the importing of Chapter information from CSV files
    /// </summary>
    internal class ChapterImporterCsv
    {
        /// <summary>
        /// The file filter value for the OpenFileDialog
        /// </summary>
        public static string FileFilter => "CSV files (*.csv;*.tsv)|*.csv;*.tsv";

        public static void Import(string filename, ref Dictionary<int, Tuple<string, TimeSpan>> importedChapters)
        {
            if (!File.Exists(filename))
            {
                throw new FileNotFoundException();
            }

            int lineNumber = 0;
            try
            {
                using (StreamReader reader = new StreamReader(filename))
                {
                    // Try guess the delimiter.
                    string contents = reader.ReadToEnd();
                    reader.DiscardBufferedData();
                    reader.BaseStream.Seek(0, SeekOrigin.Begin);
                    bool tabDelimited = contents.Split('\t').Length > contents.Split(',').Length;

                    // Parse each line.
                    while (reader.Peek() >= 0)
                    {
                        lineNumber = lineNumber + 1;
                        string line = reader.ReadLine();
                        if (!string.IsNullOrEmpty(line))
                        {
                            string[] splitContents = tabDelimited ? line.Split('\t') : line.Split(',');

                            if (splitContents.Length < 2)
                            {
                                throw new InvalidDataException(
                                    string.Format(
                                        Resources
                                            .ChaptersViewModel_UnableToImportChaptersLineDoesNotHaveAtLeastTwoColumns,
                                        lineNumber));
                            }

                            if (!int.TryParse(splitContents[0], out var chapterNumber))
                            {
                                throw new InvalidDataException(
                                    string.Format(
                                        Resources
                                            .ChaptersViewModel_UnableToImportChaptersFirstColumnMustContainOnlyIntegerNumber,
                                        lineNumber));
                            }

                            string chapterName = splitContents[1].Trim();

                            // Store the chapter name at the correct index
                            importedChapters[chapterNumber] = new Tuple<string, TimeSpan>(chapterName, TimeSpan.Zero);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                throw new GeneralApplicationException(Resources.ChaptersViewModel_UnableToImportChaptersWarning, string.Format(Resources.ChaptersViewModel_UnableToImportChaptersMalformedLineMsg, lineNumber), e);
            }
        }
    }
}
