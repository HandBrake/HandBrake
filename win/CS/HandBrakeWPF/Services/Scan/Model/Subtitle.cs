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

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model.Models;

    /// <summary>
    /// An object that represents a subtitle associated with a Title, in a DVD
    /// </summary>
    public class Subtitle
    {
        public Subtitle()
        {
        }

        public Subtitle(int sourceId)
        {
            this.SourceId = sourceId;
        }

        public Subtitle(int sourceId, int trackNumber, string language, string languageCode, SubtitleType subtitleType, bool canBurn, bool canForce, string name, bool isFakeForeignAudioScanTrack)
        {
            this.SourceId = sourceId;
            this.TrackNumber = trackNumber;
            this.Language = language;
            this.LanguageCode = languageCode;
            this.SubtitleType = subtitleType;
            this.CanBurnIn = canBurn;
            this.CanForce = canForce;
            this.Name = name;
            this.IsFakeForeignAudioScanTrack = isFakeForeignAudioScanTrack;
        }

        public int SourceId { get; set; }

        public int TrackNumber { get; set; }

        public string Language { get; set; }

        public string LanguageCode { get; set; }

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

        public bool CanBurnIn { get; set; }

        public bool CanForce { get; set; }

        public SubtitleType SubtitleType { get; set; }

        public string Name { get; set; }

        public bool IsFakeForeignAudioScanTrack { get; set; }

        public string TypeString
        {
            get
            {
                return EnumHelper<Enum>.GetDescription(this.SubtitleType);
            }
        }

        public override string ToString()
        {
            return this.IsFakeForeignAudioScanTrack ? Resources.Subtitle_ForeignAudioScan : string.Format("{0} {1}", this.TrackNumber, this.Language);
        }

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
