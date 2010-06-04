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
        /// Gets or sets the track number of this Audio Track
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// Gets or sets the language (if detected) of this Audio Track
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
        /// Gets or sets the frequency (in MHz) of this Audio Track
        /// </summary>
        public int SampleRate { get; set; }

        /// <summary>
        /// Gets or sets the bitrate (in kbps) of this Audio Track
        /// </summary>
        public int Bitrate { get; set; }

        /// <summary>
        /// Gets Display.
        /// </summary>
        public string Display
        {
            get
            {
                return this.GetDisplayString(true);
            }
        }

        /// <summary>
        /// Gets NoTrackDisplay.
        /// </summary>
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

        /// <summary>
        /// Get the Display String
        /// </summary>
        /// <param name="includeTrackNumber">
        /// The include track number.
        /// </param>
        /// <returns>
        /// A String
        /// </returns>
        private string GetDisplayString(bool includeTrackNumber)
        {
            return includeTrackNumber ? (this.TrackNumber + " " + this.Description) : this.Description;
        }
    }
}