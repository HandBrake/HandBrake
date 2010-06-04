// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object represending an AudioTrack associated with a Title, in a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.SourceData
{
    /// <summary>
    /// An object represending an AudioTrack associated with a Title, in a DVD
    /// </summary>
    public class AudioTrack
    {
        /// <summary>
        /// The track number of this Audio Track
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// The language (if detected) of this Audio Track
        /// </summary>
        public string Language { get; set; }

        public string LanguageCode { get; set; }

        public string Description { get; set; }

        /// <summary>
        /// The frequency (in MHz) of this Audio Track
        /// </summary>
        public int SampleRate { get; set; }

        /// <summary>
        /// The bitrate (in kbps) of this Audio Track
        /// </summary>
        public int Bitrate { get; set; }

        public string Display
        {
            get
            {
                return this.GetDisplayString(true);
            }
        }

        public string NoTrackDisplay
        {
            get
            {
                return this.GetDisplayString(false);
            }
        }

        /// <summary>
        /// Override of the ToString method to make this object easier to use in the UI
        /// </summary>
        /// <returns>A string formatted as: {track #} {language} ({format}) ({sub-format})</returns>
        public override string ToString()
        {
            return this.GetDisplayString(true);
        }

        private string GetDisplayString(bool includeTrackNumber)
        {
            if (includeTrackNumber)
            {
                return this.TrackNumber + " " + this.Description;
            }
            else
            {
                return this.Description;
            }
        }
    }
}