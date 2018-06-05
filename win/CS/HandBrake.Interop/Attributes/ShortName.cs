// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShortName.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Short Name for an enum value
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Attributes
{
    using System;

    /// <summary>
    ///  A Short Name for an enum value
    /// </summary>
    public class ShortName : Attribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ShortName"/> class.
        /// </summary>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public ShortName(string shortName)
        {
            this.Name = shortName;
        }

        /// <summary>
        /// Gets the short name.
        /// </summary>
        public string Name { get; private set; }
    }
}
