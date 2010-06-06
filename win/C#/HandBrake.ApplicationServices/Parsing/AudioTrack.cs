/*  AudioTrack.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Parsing
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
        /// Gets The track number of this Audio Track
        /// </summary>
        public int TrackNumber { get; private set; }

        /// <summary>
        /// Gets The language (if detected) of this Audio Track
        /// </summary>
        public string Language { get; private set; }

        /// <summary>
        /// Gets or sets LanguageCode.
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets Description.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets The primary format of this Audio Track
        /// </summary>
        public string Format { get; private set; }

        /// <summary>
        /// Gets The frequency (in MHz) of this Audio Track
        /// </summary>
        public int SampleRate { get; private set; }

        /// <summary>
        /// Gets The bitrate (in kbps) of this Audio Track
        /// </summary>
        public int Bitrate { get; private set; }

        /// <summary>
        /// Create a new Audio Track object
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        /// <param name="lang">
        /// The lang.
        /// </param>
        /// <param name="langCode">
        /// The lang code.
        /// </param>
        /// <param name="desc">
        /// The desc.
        /// </param>
        /// <param name="format">
        /// The format.
        /// </param>
        /// <param name="samplerate">
        /// The samplerate.
        /// </param>
        /// <param name="bitrate">
        /// The bitrate.
        /// </param>
        /// <returns>
        /// A new Audio Track
        /// </returns>
        public static AudioTrack CreateAudioTrack(int track, string lang, string langCode, string desc, string format, int samplerate, int bitrate)
        {
            AudioTrack newTrack = new AudioTrack
                {
                    TrackNumber = track,
                    Language = lang,
                    LanguageCode = langCode,
                    Description = desc,
                    Format = format,
                    SampleRate = samplerate,
                    Bitrate = bitrate
                };

            return newTrack;

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
                                        TrackNumber = int.Parse(track.Groups[1].Value.Trim()), 
                                        Language = track.Groups[2].Value, 
                                        Format = m.Groups[3].Value,
                                        Description = subformat, 
                                        SampleRate = int.Parse(samplerateVal), 
                                        Bitrate = int.Parse(bitrateVal),
                                        LanguageCode = iso639_2.Value.Replace("iso639-2: ", string.Empty).Replace(")", string.Empty)
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
            if (Description == null)
                return string.Format("{0} {1} ({2})", TrackNumber, Language, Format);

            return string.Format("{0} {1} ({2}) ({3})", TrackNumber, Language, Format, Description);
        }
    }
}