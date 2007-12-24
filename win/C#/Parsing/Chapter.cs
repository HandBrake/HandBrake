/*  Chapter.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.m0k.org/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace Handbrake.Parsing
{
    /// <summary>
    /// An object representing a Chapter aosciated with a Title, in a DVD
    /// </summary>
    public class Chapter
    {
        private int m_chapterNumber;
        /// <summary>
        /// The number of this Chapter, in regards to it's parent Title
        /// </summary>
        public int ChapterNumber
        {
            get
            {
                return this.m_chapterNumber;
            }
        }

        private TimeSpan m_duration;
        /// <summary>
        /// The length in time this Chapter spans
        /// </summary>
        public TimeSpan Duration
        {
            get
            {
                return this.m_duration;
            }
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {chapter #}</returns>
        public override string ToString()
        {
            return this.m_chapterNumber.ToString();
        }

        public static Chapter Parse(StringReader output)
        {
            Match m = Regex.Match(output.ReadLine(), @"^    \+ ([0-9]*): cells ([0-9]*)->([0-9]*), ([0-9]*) blocks, duration ([0-9]{2}:[0-9]{2}:[0-9]{2})");
            if (m.Success)
            {
                Chapter thisChapter = new Chapter();
                thisChapter.m_chapterNumber = int.Parse(m.Groups[1].Value.Trim().ToString());
                thisChapter.m_duration = TimeSpan.Parse(m.Groups[5].Value);
                return thisChapter;
            }
            else
                return null;
        }

        public static Chapter[] ParseList(StringReader output)
        {
            List<Chapter> chapters = new List<Chapter>();

            // this is to read the "  + chapters:" line from the buffer
            // so we can start reading the chapters themselvs
            output.ReadLine();

            while (true)
            {
                // Start of the chapter list for this Title
                Chapter thisChapter = Chapter.Parse(output);

                if (thisChapter != null)
                    chapters.Add(thisChapter);
                else
                    break;
            }
            return chapters.ToArray();
        }
    }
}
