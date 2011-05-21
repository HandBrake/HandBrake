/*  AudioTrack.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Parsing
{
    using System;

    /// <summary>
    /// An object represending an AudioTrack associated with a Title, in a DVD
    /// </summary>
    [Serializable]
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
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language} ({format}) ({sub-format})</returns>
        public override string ToString()
        {
            if (this.Description == "None Found")
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