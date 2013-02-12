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
    /// The source subtitle.
    /// </summary>
    public class SourceSubtitle
    {
        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether burned in.
        /// </summary>
        public bool BurnedIn { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether default.
        /// </summary>
        public bool Default { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether forced.
        /// </summary>
        public bool Forced { get; set; }

        /// <summary>
        ///     Gets or sets the 1-based subtitle track number. 0 means foreign audio search.
        /// </summary>
        public int TrackNumber { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// The clone.
        /// </summary>
        /// <returns>
        /// The <see cref="SourceSubtitle"/>.
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

        #endregion
    }
}