// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Subtitle.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object that represents a subtitle associated with a Title, in a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Model
{
    using System;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Utilities;

    /// <summary>
    /// An object that represents a subtitle associated with a Title, in a DVD
    /// </summary>
    public class Subtitle
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Subtitle"/> class.
        /// </summary>
        public Subtitle()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Subtitle"/> class.
        /// </summary>
        /// <param name="sourceId">
        /// The source Id.
        /// </param>
        /// <param name="trackNumber">
        /// The track number.
        /// </param>
        /// <param name="language">
        /// The language.
        /// </param>
        /// <param name="languageCode">
        /// The language code.
        /// </param>
        /// <param name="subtitleType">
        /// The subtitle type.
        /// </param>
        /// <param name="canBurn">
        /// The can Burn.
        /// </param>
        /// <param name="canForce">
        /// The can Force.
        /// </param>
        public Subtitle(int sourceId, int trackNumber, string language, string languageCode, SubtitleType subtitleType, bool canBurn, bool canForce)
        {
            this.SourceId = sourceId;
            this.TrackNumber = trackNumber;
            this.Language = language;
            this.LanguageCode = languageCode;
            this.SubtitleType = subtitleType;
            this.CanBurnIn = canBurn;
            this.CanForce = canForce;
        }

        /// <summary>
        /// Gets or sets the source id.
        /// </summary>
        public int SourceId { get; set; }

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
        /// Gets the language code clean.
        /// TODO Remove this after fixing language code.
        /// </summary>
        public string LanguageCodeClean
        {
            get
            {
                if (this.LanguageCode != null)
                {
                    return this.LanguageCode.Replace("iso639-2: ", string.Empty).Trim();
                }
                return string.Empty;
            }
        }

        /// <summary>
        /// Gets a value indicating whether can burn in.
        /// </summary>
        [XmlIgnore]
        public bool CanBurnIn { get; private set; }

        /// <summary>
        /// Gets a value indicating whether can force.
        /// </summary>
        [XmlIgnore]
        public bool CanForce { get; private set; }

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
                return EnumHelper<Enum>.GetDescription(this.SubtitleType);
            }
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language}</returns>
        public override string ToString()
        {
            return this.SubtitleType == SubtitleType.ForeignAudioSearch ? "Foreign Audio Scan" : string.Format("{0} {1} ({2})", this.TrackNumber, this.Language, this.TypeString);
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
        public bool Equals(Subtitle other)
        {
            if (ReferenceEquals(null, other))
            {
                return false;
            }
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            return other.TrackNumber == this.TrackNumber && object.Equals(other.Language, this.Language) && object.Equals(other.LanguageCode, this.LanguageCode) && object.Equals(other.SubtitleType, this.SubtitleType);
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
            
            if (obj.GetType() != typeof(Subtitle))
            {
                return false;
            }

            return this.Equals((Subtitle)obj);
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
                result = (result * 397) ^ this.SubtitleType.GetHashCode();
                return result;
            }
        }
    }
}