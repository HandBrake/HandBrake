/*  Subtitle.cs $
 	
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
    /// An object that represents a subtitle associated with a Title, in a DVD
    /// </summary>
    public class Subtitle
    {
        private int m_trackNumber;
        /// <summary>
        /// The track number of this Subtitle
        /// </summary>
        public int TrackNumber
        {
            get
            {
                return this.m_trackNumber;
            }
        }

        private string m_language;
        /// <summary>
        /// The language (if detected) of this Subtitle
        /// </summary>
        public string Language
        {
            get
            {
                return this.m_language;
            }
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language}</returns>
        public override string ToString()
        {
            return string.Format("{0} {1}", this.m_trackNumber, this.m_language);
        }

        public static Subtitle Parse(StringReader output)
        {
            string curLine = output.ReadLine();
            Match m = Regex.Match(curLine, @"^    \+ ([0-9]*), ([A-Za-z]*) \((.*)\)");
            if (m.Success && !curLine.Contains("HandBrake has exited."))
            {
                Subtitle thisSubtitle = new Subtitle();
                thisSubtitle.m_trackNumber = int.Parse(m.Groups[1].Value.Trim().ToString());
                thisSubtitle.m_language = m.Groups[2].Value;
                return thisSubtitle;
            }
            else
                return null;
        }

        public static Subtitle[] ParseList(StringReader output)
        {
            List<Subtitle> subtitles = new List<Subtitle>();
            while ((char)output.Peek() != '+')
            {
                Subtitle thisSubtitle = Subtitle.Parse(output);

                if (thisSubtitle != null)
                    subtitles.Add(thisSubtitle);
                else
                    break;
            }
            return subtitles.ToArray();
        }
    }
}
