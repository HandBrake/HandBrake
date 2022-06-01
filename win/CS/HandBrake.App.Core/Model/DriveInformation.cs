// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DriveInformation.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Information about a DVD drive
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Model
{
    /// <summary>
    /// Information about a DVD drive
    /// </summary>
    public class DriveInformation
    {
        /// <summary>
        /// Gets or sets A Unique ID That represents this model.
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets The Drive Volume Name
        /// </summary>
        public string VolumeLabel { get; set; }

        /// <summary>
        /// Gets or sets The Root Directory
        /// </summary>
        public string RootDirectory { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this is a BluRay Drive.
        /// </summary>
        public bool IsBluRay { get; set; }

        /// <summary>
        /// Returns  "Drive" + Id  (e.g  Drive2)
        /// </summary>
        /// <returns>
        /// A String that contains "Drive" and it's ID
        /// </returns>
        public override string ToString()
        {
            return "Drive" + this.Id;
        }
    }
}