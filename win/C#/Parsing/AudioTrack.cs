/*  AudioTrack.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Parsing
{
    using System.Collections.Generic;
    using System.IO;
    using System.Text.RegularExpressions;

    /// <summary>
    /// An object represending an AudioTrack associated with a Title, in a DVD
    /// </summary>
    public class AudioTrack
    {
        /// <summary>
        /// The Track bitrate
        /// </summary>
        private int bitrate;

        /// <summary>
        /// The track format
        /// </summary>
        private string format;

        /// <summary>
        /// The Frequency
        /// </summary>
        private int frequency;

        /// <summary>
        /// Track Language
        /// </summary>
        private string language;

        /// <summary>
        /// Sub Format
        /// </summary>
        private string subFormat;

        /// <summary>
        /// Track Number
        /// </summary>
        private int trackNumber;

        /// <summary>
        /// The ISO639_2 code
        /// </summary>
        private string iso639_2;

        /// <summary>
        /// Gets The track number of this Audio Track
        /// </summary>
        public int TrackNumber
        {
            get { return trackNumber; }
        }

        /// <summary>
        /// Gets The language (if detected) of this Audio Track
        /// </summary>
        public string Language
        {
            get { return language; }
        }

        /// <summary>
        /// Gets The primary format of this Audio Track
        /// </summary>
        public string Format
        {
            get { return format; }
        }

        /// <summary>
        /// Gets Additional format info for this Audio Track
        /// </summary>
        public string SubFormat
        {
            get { return subFormat; }
        }

        /// <summary>
        /// Gets The frequency (in MHz) of this Audio Track
        /// </summary>
        public int Frequency
        {
            get { return frequency; }
        }

        /// <summary>
        /// Gets The bitrate (in kbps) of this Audio Track
        /// </summary>
        public int Bitrate
        {
            get { return bitrate; }
        }

        /// <summary>
        /// Gets ISO639_2.
        /// </summary>
        public string ISO639_2
        {
            get { return iso639_2; }
        }

        /// <summary>
        /// Parse the CLI input to an Audio Track object
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// An Audio Track obkect
        /// </returns>
        public static AudioTrack Parse(StringReader output)
        {
            string audioTrack = output.ReadLine();
            Match m = Regex.Match(audioTrack, @"^    \+ ([0-9]*), ([A-Za-z0-9,\s]*) \((.*)\) \((.*)\)");
            Match track = Regex.Match(audioTrack, @"^    \+ ([0-9]*), ([A-Za-z0-9,\s]*) \((.*)\)"); // ID and Language
            Match iso639_2 = Regex.Match(audioTrack, @"iso639-2: ([a-zA-Z]*)\)");
            Match samplerate = Regex.Match(audioTrack, @"([0-9]*)Hz");
            Match bitrate = Regex.Match(audioTrack, @"([0-9]*)bps");

            string subformat = m.Groups[4].Value.Trim().Contains("iso639") ? null : m.Groups[4].Value;
            string samplerateVal = samplerate.Success ? samplerate.Groups[0].Value.Replace("Hz", string.Empty).Trim() : "0";
            string bitrateVal = bitrate.Success ? bitrate.Groups[0].Value.Replace("bps", string.Empty).Trim() : "0";

            if (track.Success)
            {
                var thisTrack = new AudioTrack
                                    {
                                        trackNumber = int.Parse(track.Groups[1].Value.Trim()), 
                                        language = track.Groups[2].Value, 
                                        format = m.Groups[3].Value, 
                                        subFormat = subformat, 
                                        frequency = int.Parse(samplerateVal), 
                                        bitrate = int.Parse(bitrateVal), 
                                        iso639_2 =
                                            iso639_2.Value.Replace("iso639-2: ", string.Empty).Replace(")", string.Empty)
                                    };
                return thisTrack;
            }

            return null;
        }

        /// <summary>
        /// Pase a list of audio tracks
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// An array of audio tracks
        /// </returns>
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

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language} ({format}) ({sub-format})</returns>
        public override string ToString()
        {
            if (subFormat == null)
                return string.Format("{0} {1} ({2})", trackNumber, language, format);

            return string.Format("{0} {1} ({2}) ({3})", trackNumber, language, format, subFormat);
        }
    }
}