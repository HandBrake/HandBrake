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
    public class SourceSubtitle
    {
        /// <summary>
        /// Gets or sets the 1-based subtitle track number. 0 means foriegn audio search.
        /// </summary>
        public int TrackNumber { get; set; }
        public bool Default { get; set; }
        public bool Forced { get; set; }
        public bool BurnedIn { get; set; }

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
