using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace Handbrake.Parsing
{
    /// <summary>
    /// An object represending an AudioTrack associated with a Title, in a DVD
    /// </summary>
    public class AudioTrack
    {
        private int m_trackNumber;
        /// <summary>
        /// The track number of this Audio Track
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
        /// The language (if detected) of this Audio Track
        /// </summary>
        public string Language
        {
            get
            {
                return this.m_language;
            }
        }

        private string m_format;
        /// <summary>
        /// The primary format of this Audio Track
        /// </summary>
        public string Format
        {
            get
            {
                return this.m_format;
            }
        }

        private string m_subFormat;
        /// <summary>
        /// Additional format info for this Audio Track
        /// </summary>
        public string SubFormat
        {
            get
            {
                return this.m_subFormat;
            }
        }

        private int m_frequency;
        /// <summary>
        /// The frequency (in MHz) of this Audio Track
        /// </summary>
        public int Frequency
        {
            get
            {
                return this.m_frequency;
            }
        }

        private int m_bitrate;
        /// <summary>
        /// The bitrate (in kbps) of this Audio Track
        /// </summary>
        public int Bitrate
        {
            get
            {
                return this.m_bitrate;
            }
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language} ({format}) ({sub-format})</returns>
        public override string ToString()
        {
            return string.Format("{0} {1} ({2}) ({3})", this.m_trackNumber, this.m_language, this.m_format, this.m_subFormat);
        }

        public static AudioTrack Parse(StringReader output)
        {
            Match m = Regex.Match(output.ReadLine(), @"^    \+ ([0-9]*), ([A-Za-z0-9]*) \((.*)\) \((.*)\), ([0-9]*)Hz, ([0-9]*)bps");
            if (m.Success)
            {
                AudioTrack thisTrack = new AudioTrack();
                thisTrack.m_trackNumber = int.Parse(m.Groups[1].Value.Trim().ToString());
                thisTrack.m_language = m.Groups[2].Value;
                thisTrack.m_format = m.Groups[3].Value;
                thisTrack.m_subFormat = m.Groups[4].Value;
                thisTrack.m_frequency = int.Parse(m.Groups[5].Value.Trim().ToString());
                thisTrack.m_bitrate = int.Parse(m.Groups[6].Value.Trim().ToString());
                return thisTrack;
            }
            else
            {
                return null;
            }
        }

        public static AudioTrack[] ParseList(StringReader output)
        {
            List<AudioTrack> tracks = new List<AudioTrack>();
            while (true)
            {
                AudioTrack thisTrack = AudioTrack.Parse(output);
                if (thisTrack != null)
                {
                    tracks.Add(thisTrack);
                }
                else
                {
                    break;
                }
            }
            return tracks.ToArray();
        }
    }
}
