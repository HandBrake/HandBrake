// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBRate.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Represents a rate in HandBrake: audio sample rate or video framerate.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    /// <summary>
    /// Represents a rate in HandBrake: audio sample rate or video framerate.
    /// </summary>
    public class HBRate
    {
        /// <summary>
        /// Gets or sets the name to use for this rate.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the raw rate.
        /// </summary>
        public int Rate { get; set; }
    }
}
