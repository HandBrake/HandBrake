// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Audio.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object represending an AudioTrack associated with a Title, in a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Model
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

            return string.Format("{0} {1}", this.TrackNumber, this.Description);
        }

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="other">
        /// The other.
        /// </param>
        /// <returns>
        /// The System.Boolean.
        /// </returns>
        public bool Equals(Audio other)
        {
            if (ReferenceEquals(null, other))
            {
                return false;
            }

            if (ReferenceEquals(this, other))
            {
                return true;
            }

            return other.TrackNumber == this.TrackNumber && object.Equals(other.Language, this.Language) && object.Equals(other.LanguageCode, this.LanguageCode) && object.Equals(other.Format, this.Format);
        }

        /// <summary>
        /// Determines whether the specified <see cref="T:System.Object"/> is equal to the current <see cref="T:System.Object"/>.
        /// </summary>
        /// <returns>
        /// true if the specified <see cref="T:System.Object"/> is equal to the current <see cref="T:System.Object"/>; otherwise, false.
        /// </returns>
        /// <param name="obj">The <see cref="T:System.Object"/> to compare with the current <see cref="T:System.Object"/>. </param><filterpriority>2</filterpriority>
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != typeof(Audio))
            {
                return false;
            }

            return this.Equals((Audio)obj);
        }

        /// <summary>
        /// Serves as a hash function for a particular type. 
        /// </summary>
        /// <returns>
        /// A hash code for the current <see cref="T:System.Object"/>.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override int GetHashCode()
        {
            unchecked
            {
                int result = this.TrackNumber;
                result = (result * 397) ^ (this.Language != null ? this.Language.GetHashCode() : 0);
                result = (result * 397) ^ (this.LanguageCode != null ? this.LanguageCode.GetHashCode() : 0);
                result = (result * 397) ^ (this.Format != null ? this.Format.GetHashCode() : 0);
                return result;
            }
        }
    }
}