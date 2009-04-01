/*  AudioTrack.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

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
        private int m_bitrate;
        private string m_format;
        private int m_frequency;
        private string m_language;
        private string m_subFormat;
        private int m_trackNumber;

        /// <summary>
        /// The track number of this Audio Track
        /// </summary>
        public int TrackNumber
        {
            get { return m_trackNumber; }
        }

        /// <summary>
        /// The language (if detected) of this Audio Track
        /// </summary>
        public string Language
        {
            get { return m_language; }
        }

        /// <summary>
        /// The primary format of this Audio Track
        /// </summary>
        public string Format
        {
            get { return m_format; }
        }

        /// <summary>
        /// Additional format info for this Audio Track
        /// </summary>
        public string SubFormat
        {
            get { return m_subFormat; }
        }

        /// <summary>
        /// The frequency (in MHz) of this Audio Track
        /// </summary>
        public int Frequency
        {
            get { return m_frequency; }
        }

        /// <summary>
        /// The bitrate (in kbps) of this Audio Track
        /// </summary>
        public int Bitrate
        {
            get { return m_bitrate; }
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language} ({format}) ({sub-format})</returns>
        public override string ToString()
        {
            if (m_subFormat == null)
                return string.Format("{0} {1} ({2})", m_trackNumber, m_language, m_format);
            
            return string.Format("{0} {1} ({2}) ({3})", m_trackNumber, m_language, m_format, m_subFormat);
        }

        public static AudioTrack Parse(StringReader output)
        {
            String audio_track = output.ReadLine();
            Match m = Regex.Match(audio_track,
                                  @"^    \+ ([0-9]*), ([A-Za-z0-9]*) \((.*)\) \((.*)\), ([0-9]*)Hz, ([0-9]*)bps");
            Match y = Regex.Match(audio_track, @"^    \+ ([0-9]*), ([A-Za-z0-9]*) \((.*)\)");
            if (m.Success)
            {
                var thisTrack = new AudioTrack
                                    {
                                        m_trackNumber = int.Parse(m.Groups[1].Value.Trim()),
                                        m_language = m.Groups[2].Value,
                                        m_format = m.Groups[3].Value,
                                        m_subFormat = m.Groups[4].Value,
                                        m_frequency = int.Parse(m.Groups[5].Value.Trim()),
                                        m_bitrate = int.Parse(m.Groups[6].Value.Trim())
                                    };
                return thisTrack;
            }
            
            if (y.Success)
            {
                var thisTrack = new AudioTrack
                                    {
                                        m_trackNumber = int.Parse(y.Groups[1].Value.Trim()),
                                        m_language = y.Groups[2].Value,
                                        m_format = y.Groups[3].Value
                                    };
                return thisTrack;
            }
            
            return null;
        }

        public static AudioTrack[] ParseList(StringReader output)
        {
            var tracks = new List<AudioTrack>();
            while (true)
            {
                AudioTrack thisTrack = Parse(output);
                if (thisTrack != null)
                    tracks.Add(thisTrack);
                else
                    break;
            }
            return tracks.ToArray();
        }
    }
}