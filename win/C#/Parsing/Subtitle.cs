using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

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

        public static Subtitle Parse(StreamReader output)
        {
            string curLine = output.ReadLine();
            if (!curLine.Contains("HandBrake has exited."))
            {
                Subtitle thisSubtitle = new Subtitle();
                string[] splitter = curLine.Split(new string[] { "    + ", ", " }, StringSplitOptions.RemoveEmptyEntries);
                thisSubtitle.m_trackNumber = int.Parse(splitter[0]);
                thisSubtitle.m_language = splitter[1];
                return thisSubtitle;
            }
            else
            {
                return null;
            }
        }

        public static Subtitle[] ParseList(StreamReader output)
        {
            List<Subtitle> subtitles = new List<Subtitle>();
            while ((char)output.Peek() != '+') // oh glorious hack, serve me well
            {
                Subtitle thisSubtitle = Subtitle.Parse(output);

                if (thisSubtitle != null)
                {
                    subtitles.Add(thisSubtitle);
                }
                else
                {
                    break;
                }
            }
            return subtitles.ToArray();
        }
    }
}
