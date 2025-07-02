// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBMixdown.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb mixdown.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Encoders
{
    /// <summary>
    /// The hb mixdown.
    /// </summary>
    public class HBColourRange
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HBColourRange"/> class.
        /// </summary>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="id">
        /// The id.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public HBColourRange(string displayName, int id, string shortName)
        {
            this.DisplayName = displayName;
            this.Id = id;
            this.ShortName = shortName;
        }

        /// <summary>
        /// Gets the display name.
        /// </summary>
        public string DisplayName { get; private set; }

        /// <summary>
        /// Gets the id.
        /// </summary>
        public int Id { get; private set; }

        /// <summary>
        /// Gets the short name.
        /// </summary>
        public string ShortName { get; private set; }
    }
}