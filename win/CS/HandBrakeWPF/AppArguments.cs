// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AppArguments.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AppArguments type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF
{
    /// <summary>
    /// The app arguments.
    /// </summary>
    public class AppArguments
    {
        /// <summary>
        /// Gets or sets a value indicating whether is instant hand brake.
        /// </summary>
        public static bool IsInstantHandBrake { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether enable lib hb.
        /// </summary>
        public static bool UseLibHb { get; set; }
    }
}
