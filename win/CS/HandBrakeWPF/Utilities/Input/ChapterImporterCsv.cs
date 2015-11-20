using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HandBrakeWPF.Utilities.Input
{
    using HandBrakeWPF.Exceptions;
    using HandBrakeWPF.Properties;

    using Microsoft.VisualBasic.FileIO;

    /// <summary>
    /// Handles the importing of Chapter information from CSV files
    /// </summary>
    internal class ChapterImporterCsv
    {
        public static string FileFilter => "CSV files (*.csv;*.tsv)|*.csv;*.tsv";

        public static void Import(string filename, ref Dictionary<int, Tuple<string, TimeSpan>> importedChapters)
        {
            using (TextFieldParser csv = new TextFieldParser(filename)
            {
                CommentTokens = new[] { "#" }, // Comment lines
                Delimiters = new[] { ",", "\t", ";", ":" }, // Support all of these common delimeter types
                HasFieldsEnclosedInQuotes = true, // Assume that our data will be properly escaped
                TextFieldType = FieldType.Delimited,
                TrimWhiteSpace = true // Remove excess whitespace from ends of imported values
            })
            {
                while (!csv.EndOfData)
                {
                    try
                    {
                        // Only read the first two columns, anything else will be ignored but will not raise an error
                        var row = csv.ReadFields();
                        if (row == null || row.Length < 2) // null condition happens if the file is somehow corrupt during reading
                            throw new MalformedLineException(Resources.ChaptersViewModel_UnableToImportChaptersLineDoesNotHaveAtLeastTwoColumns, csv.LineNumber);

                        int chapterNumber;
                        if (!int.TryParse(row[0], out chapterNumber))
                            throw new MalformedLineException(Resources.ChaptersViewModel_UnableToImportChaptersFirstColumnMustContainOnlyIntegerNumber, csv.LineNumber);

                        // Store the chapter name at the correct index
                        importedChapters[chapterNumber] = new Tuple<string, TimeSpan>(row[1]?.Trim(), TimeSpan.Zero);
                    }
                    catch (MalformedLineException mlex)
                    {
                        throw new GeneralApplicationException(
                            Resources.ChaptersViewModel_UnableToImportChaptersWarning,
                            string.Format(Resources.ChaptersViewModel_UnableToImportChaptersMalformedLineMsg, mlex.LineNumber),
                            mlex);
                    }
                }
            }
        }
    }
}
