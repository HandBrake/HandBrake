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
        /// Initializes a new instance of the <see cref="HBRate"/> class.
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <param name="rate">
        /// The rate.
        /// </param>
        public HBRate(string name, int rate)
        {
            this.Name = name;
            this.Rate = rate;
        }

        /// <summary>
        /// Gets the name to use for this rate.
        /// </summary>
        public string Name { get; private set; }

        /// <summary>
        /// Gets the raw rate.
        /// </summary>
        public int Rate { get; private set; }
    }
}
