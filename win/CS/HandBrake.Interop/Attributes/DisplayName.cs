// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DisplayName.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Display Name for an enum value
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Attributes
{
    using System;

    /// <summary>
    ///  A Short Name for an enum value
    /// </summary>
    public class DisplayName : Attribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DisplayName"/> class.
        /// </summary>
        /// <param name="displayName">
        /// The name name.
        /// </param>
        public DisplayName(string displayName)
        {
            this.Name = displayName;
        }

        /// <summary>
        /// Gets the short name.
        /// </summary>
        public string Name { get; private set; }
    }
}
