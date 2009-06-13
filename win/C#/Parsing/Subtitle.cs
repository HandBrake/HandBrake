/*  Subtitle.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

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
        private string m_language;
        private int m_trackNumber;
        private string m_type;
        private string m_typecode;

        /// <summary>
        /// The track number of this Subtitle
        /// </summary>
        public int TrackNumber
        {
            get { return m_trackNumber; }
        }

        /// <summary>
        /// The language (if detected) of this Subtitle
        /// </summary>
        public string Language
        {
            get { return m_language; }
        }

        /// <summary>
        /// Langauage Code
        /// </summary>
        public string LanguageCode
        {
            get { return m_typecode; }
        }


        /// <summary>
        /// Subtitle Type
        /// </summary>
        public string Type
        {
            get { return m_type; }
        }


        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language}</returns>
        public override string ToString()
        {
            return string.Format("{0} {1} ({2})", m_trackNumber, m_language, m_type);
        }

        public static Subtitle Parse(StringReader output)
        {
            string curLine = output.ReadLine();

            Match m = Regex.Match(curLine, @"^    \+ ([0-9]*), ([A-Za-z, ]*) \((.*)\) \(([a-zA-Z]*)\)");
            if (m.Success && !curLine.Contains("HandBrake has exited."))
            {
                var thisSubtitle = new Subtitle
                                       {
                                           m_trackNumber = int.Parse(m.Groups[1].Value.Trim()),
                                           m_language = m.Groups[2].Value,
                                           m_typecode = m.Groups[3].Value,
                                           m_type = m.Groups[4].Value
                                       };
                return thisSubtitle;
            }
            return null;
        }

        public static Subtitle[] ParseList(StringReader output)
        {
            var subtitles = new List<Subtitle>();
            while ((char) output.Peek() != '+')
            {
                Subtitle thisSubtitle = Parse(output);

                if (thisSubtitle != null)
                    subtitles.Add(thisSubtitle);
                else
                    break;
            }
            return subtitles.ToArray();
        }
    }
}