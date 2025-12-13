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

    using HandBrake.App.Core.Exceptions;
    using HandBrake.App.Core.Utilities;

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
                IList<string[]> fileConents = CsvHelper.ParseFile(filename);
                foreach (string[] row in fileConents)
                {
                    if (row.Length < 2)
                    {
                        throw new InvalidDataException(string.Format(Resources.ChaptersViewModel_UnableToImportChaptersLineDoesNotHaveAtLeastTwoColumns, lineNumber));
                    }

                    if (!int.TryParse(row[0], out var chapterNumber))
                    {
                        throw new InvalidDataException(string.Format(Resources.ChaptersViewModel_UnableToImportChaptersFirstColumnMustContainOnlyIntegerNumber, lineNumber));
                    }

                    string chapterName = row[1].Trim();
                    
                    // Store the chapter name at the correct index
                    importedChapters[chapterNumber] = new Tuple<string, TimeSpan>(chapterName, TimeSpan.Zero);
                }
            }
            catch (Exception e)
            {
                throw new GeneralApplicationException(Resources.ChaptersViewModel_UnableToImportChaptersWarning, string.Format(Resources.ChaptersViewModel_UnableToImportChaptersMalformedLineMsg, lineNumber), e);
            }
        }
    }
}
