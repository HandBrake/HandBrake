// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AdvancedChoice.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AdvancedChoice type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    /// <summary>
    /// Advanced Choce model for the Advanced Panel
    /// </summary>
    public class AdvancedChoice
    {
        /// <summary>
        /// Gets or sets a value indicating whether the given choice is default.
        /// </summary>
        public bool IsDefault { get; set; }

        /// <summary>
        /// Gets or sets the UI label for the choice.
        /// </summary>
        public string Label { get; set; }

        /// <summary>
        /// Gets or sets the value on the options string for the choice.
        /// </summary>
        public string Value { get; set; }
    }
}
