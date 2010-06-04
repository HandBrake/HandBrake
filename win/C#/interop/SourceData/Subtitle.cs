/*  Subtitle.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace HandBrake.SourceData
{
    /// <summary>
    /// An object that represents a subtitle associated with a Title, in a DVD
    /// </summary>
    public class Subtitle
    {
        /// <summary>
        /// The track number of this Subtitle
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// The language (if detected) of this Subtitle
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Langauage Code
        /// </summary>
        public string LanguageCode { get; set; }

        public SubtitleType SubtitleType { get; set; }

        /// <summary>
        /// Subtitle Type
        /// </summary>
        public string TypeString
        {
            get
            {
                if (this.SubtitleType == SubtitleType.Picture)
                {
                    return "Bitmap";
                }
                else
                {
                    return "Text";
                }
            }
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language}</returns>
        public override string ToString()
        {
            return string.Format("{0} {1} ({2})", this.TrackNumber, this.Language, this.TypeString);
        }

        public string Display
        {
            get
            {
                return this.ToString();
            }
        }
    }
}