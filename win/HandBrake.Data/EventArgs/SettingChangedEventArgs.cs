// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SettingChangedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The setting changed event args.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.EventArgs
{
    /// <summary>
    /// The setting changed event args.
    /// </summary>
    public class SettingChangedEventArgs
    {
        /// <summary>
        /// Gets or sets the key.
        /// </summary>
        public string Key { get; set; }

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public object Value { get; set; }
    }
}
