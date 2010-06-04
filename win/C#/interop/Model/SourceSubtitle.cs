// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceSubtitle.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SourceSubtitle type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    /// <summary>
    /// A Source Subtitle
    /// </summary>
    public class SourceSubtitle
    {
        /// <summary>
        /// Gets or sets the 1-based subtitle track number. 0 means foriegn audio search.
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the subtitle is Default.
        /// </summary>
        public bool Default { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the subtitle is Forced.
        /// </summary>
        public bool Forced { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the subtitle is BurnedIn.
        /// </summary>
        public bool BurnedIn { get; set; }

        /// <summary>
        /// Clone the Source Subtitle
        /// </summary>
        /// <returns>
        /// A Source Subtitle
        /// </returns>
        public SourceSubtitle Clone()
        {
            return new SourceSubtitle
            {
                TrackNumber = this.TrackNumber,
                Default = this.Default,
                Forced = this.Forced,
                BurnedIn = this.BurnedIn
            };
        }
    }
}
