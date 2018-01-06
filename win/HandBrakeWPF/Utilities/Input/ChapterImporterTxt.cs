// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChapterImporterTxt.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Imports chapter markers in the ChaptersDb.org TXT format
//   More info: http://www.chapterdb.org/docs
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities.Input
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using HandBrakeWPF.Helpers;

    /// <summary>
    /// Imports chapter markers in the ChaptersDb.org TXT format
    /// More info: http://www.chapterdb.org/docs
    /// </summary>
    public class ChapterImporterTxt
    {
        /// <summary>
        /// The file filter value for the OpenFileDialog
        /// </summary>
        public static string FileFilter => "Text files (*.txt)|*.txt";

        /// <summary>
        /// Imports all chapter information from the given <see cref="filename"/> into the <see cref="chapterMap"/> dictionary.
        /// </summary>
        /// <param name="filename">The full path and filename of the chapter marker file to import</param>
        /// <param name="chapterMap">The dictionary that should be populated with parsed chapter markers</param>
        public static void Import(string filename, ref Dictionary<int, Tuple<string, TimeSpan>> chapterMap)
        {
            using (var file = new StreamReader(filename))
            {
                // Indexing is 1-based
                int chapterMapIdx = 1;
                TimeSpan prevChapterStart = TimeSpan.Zero;

                while (!file.EndOfStream)
                {
                    // Read the lines in pairs, the duration is always first then the chapter name
                    var chapterStartRaw = file.ReadLine();
                    var chapterName = file.ReadLine();

                    // If either of the values is null then the end of the file has been reached and we need to terminate
                    if (chapterName == null || chapterStartRaw == null)
                        break;

                    // Split the values on '=' and take the left side
                    chapterName = chapterName.Split(new[] { '=' }, 2).LastOrDefault();
                    chapterStartRaw = chapterStartRaw.Split(new[] { '=' }, 2).LastOrDefault();

                    // Parse the time
                    if (!string.IsNullOrWhiteSpace(chapterStartRaw))
                    {
                        var chapterStart = TimeSpanHelper.ParseChapterTimeStart(chapterStartRaw);

                        // If we're past the first chapter in the file then calculate the duration for the previous chapter
                        if (chapterMapIdx > 1)
                        {
                            var old = chapterMap[chapterMapIdx - 1];
                            chapterMap[chapterMapIdx - 1] = new Tuple<string, TimeSpan>(old.Item1, chapterStart - prevChapterStart);
                        }

                        prevChapterStart = chapterStart;
                    }

                    // Save the chapter info, we calculate the duration in the next iteration (look back)
                    chapterMap[chapterMapIdx++] = new Tuple<string, TimeSpan>(chapterName, TimeSpan.Zero);
                }
            }
        }
    }
}
