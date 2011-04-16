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
    public class Audio
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Audio"/> class.
        /// </summary>
        public Audio()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Audio"/> class.
        /// </summary>
        /// <param name="trackNumber">
        /// The track number.
        /// </param>
        /// <param name="language">
        /// The language.
        /// </param>
        /// <param name="languageCode">
        /// The language code.
        /// </param>
        /// <param name="description">
        /// The description.
        /// </param>
        /// <param name="format">
        /// The format.
        /// </param>
        /// <param name="sampleRate">
        /// The sample rate.
        /// </param>
        /// <param name="bitrate">
        /// The bitrate.
        /// </param>
        public Audio(int trackNumber, string language, string languageCode, string description, string format, int sampleRate, int bitrate)
        {
            this.TrackNumber = trackNumber;
            this.Language = language;
            this.LanguageCode = languageCode;
            this.Description = description;
            this.Format = format;
            this.SampleRate = sampleRate;
            this.Bitrate = bitrate;
        }

        /// <summary>
        /// Gets A Dummy Not Found Track
        /// </summary>
        public static Audio NoneFound
        {
            get
            {
                return new Audio { Description = "None Found" };
            }
        }

        /// <summary>
        /// Gets or sets The track number of this Audio Track
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// Gets or sets The language (if detected) of this Audio Track
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets LanguageCode.
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets Description.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets The primary format of this Audio Track
        /// </summary>
        public string Format { get; set; }

        /// <summary>
        /// Gets or sets The frequency (in MHz) of this Audio Track
        /// </summary>
        public int SampleRate { get; set; }

        /// <summary>
        /// Gets or sets The bitrate (in kbps) of this Audio Track
        /// </summary>
        public int Bitrate { get; set; }

        /// <summary>
        /// Parse the CLI input to an Audio Track object
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// An Audio Track obkect
        /// </returns>
        public static Audio Parse(StringReader output)
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
                var thisTrack = new Audio
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
        public static Audio[] ParseList(StringReader output)
        {
            var tracks = new List<Audio>();
            while (true)
            {
                Audio thisTrack = Parse(output);
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
            if (this.Description == NoneFound.Description)
            {
                return this.Description;
            }

            if (this.Description == null)
            {
                return string.Format("{0} {1} ({2})", this.TrackNumber, this.Language, this.Format);
            }

            return string.Format("{0} {1} ({2}) ({3})", this.TrackNumber, this.Language, this.Format, this.Description);
        }
    }
}