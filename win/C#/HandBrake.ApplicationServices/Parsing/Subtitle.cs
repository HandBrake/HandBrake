/*  Subtitle.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Parsing
{
    using System.Collections.Generic;
    using System.IO;
    using System.Text.RegularExpressions;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// An object that represents a subtitle associated with a Title, in a DVD
    /// </summary>
    public class Subtitle
    {
        /// <summary>
        /// Gets or sets the track number of this Subtitle
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// Gets or sets the The language (if detected) of this Subtitle
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the Langauage Code
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets the Subtitle Type
        /// </summary>
        public SubtitleType SubtitleType { get; set; }

        /// <summary>
        /// Gets Subtitle Type
        /// </summary>
        public string TypeString
        {
            get
            {
                return this.SubtitleType == SubtitleType.Picture ? "Bitmap" : "Text";
            }
        }

        /// <summary>
        /// Parse the input strings related to subtitles
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// A Subitle object
        /// </returns>
        public static Subtitle Parse(StringReader output)
        {
            string curLine = output.ReadLine();

            Match m = Regex.Match(curLine, @"^    \+ ([0-9]*), ([A-Za-z, ]*) \((.*)\) \(([a-zA-Z]*)\)");
            if (m.Success && !curLine.Contains("HandBrake has exited."))
            {
                var thisSubtitle = new Subtitle
                                       {
                                           TrackNumber = int.Parse(m.Groups[1].Value.Trim()), 
                                           Language = m.Groups[2].Value, 
                                           LanguageCode = m.Groups[3].Value, 
                                           SubtitleType = m.Groups[4].Value.Contains("Text") ? SubtitleType.Text : SubtitleType.Picture
                                       };
                return thisSubtitle;
            }
            return null;
        }

        /// <summary>
        /// Parse a list of Subtitle tracks from an input string.
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// An array of Subtitle objects
        /// </returns>
        public static IEnumerable<Subtitle> ParseList(StringReader output)
        {
            var subtitles = new List<Subtitle>();
            while ((char)output.Peek() != '+')
            {
                Subtitle thisSubtitle = Parse(output);

                if (thisSubtitle != null)
                    subtitles.Add(thisSubtitle);
                else
                    break;
            }
            return subtitles.ToArray();
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language}</returns>
        public override string ToString()
        {
            return string.Format("{0} {1} ({2})", TrackNumber, Language, TypeString);
        }
    }
}