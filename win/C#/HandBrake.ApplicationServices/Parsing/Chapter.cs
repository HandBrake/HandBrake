/*  Chapter.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Parsing
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text.RegularExpressions;

    /// <summary>
    /// An object representing a Chapter aosciated with a Title, in a DVD
    /// </summary>
    public class Chapter
    {
        /// <summary>
        /// Gets or sets The number of this Chapter, in regards to it's parent Title
        /// </summary>
        public int ChapterNumber { get; set; }

        /// <summary>
        /// Gets or sets ChapterName.
        /// </summary>
        public string ChapterName { get; set; }

        /// <summary>
        /// Gets or sets The length in time this Chapter spans
        /// </summary>
        public TimeSpan Duration { get; set; }

        /// <summary>
        /// Parse a CLI string to a Chapter object
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// A chapter Object
        /// </returns>
        public static Chapter Parse(StringReader output)
        {
            // TODO add support for reading chapter names to the regex.
            Match m = Regex.Match(
                                  output.ReadLine(),
                                  @"^    \+ ([0-9]*): cells ([0-9]*)->([0-9]*), ([0-9]*) blocks, duration ([0-9]{2}:[0-9]{2}:[0-9]{2})");
            if (m.Success)
            {
                var thisChapter = new Chapter
                                      {
                                          ChapterNumber = int.Parse(m.Groups[1].Value.Trim()), 
                                          ChapterName = string.Empty,
                                          Duration = TimeSpan.Parse(m.Groups[5].Value)
                                      };
                return thisChapter;
            }
            return null;
        }

        /// <summary>
        /// Prase a list of strings / chatpers
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// An array of chapter objects
        /// </returns>
        public static Chapter[] ParseList(StringReader output)
        {
            var chapters = new List<Chapter>();

            // this is to read the "  + chapters:" line from the buffer
            // so we can start reading the chapters themselvs
            output.ReadLine();

            while (true)
            {
                // Start of the chapter list for this Title
                Chapter thisChapter = Parse(output);

                if (thisChapter != null)
                    chapters.Add(thisChapter);
                else
                    break;
            }
            return chapters.ToArray();
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {chapter #}</returns>
        public override string ToString()
        {
            return ChapterNumber.ToString();
        }
    }
}