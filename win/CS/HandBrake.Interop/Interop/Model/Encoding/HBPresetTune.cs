// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBPresetTune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HBPresetTune type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Model.Encoding
{
    /// <summary>
    /// The hb preset tune.
    /// </summary>
    public class HBPresetTune
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HBPresetTune"/> class. 
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <param name="shortName">
        /// The short Name.
        /// </param>
        public HBPresetTune(string name, string shortName)
        {
            this.Name = name;
            this.ShortName = shortName;
        }

        /// <summary>
        /// Gets the name.
        /// </summary>
        public string Name { get; private set; }

        /// <summary>
        /// Gets the short name.
        /// </summary>
        public string ShortName { get; private set; }
    }
}
