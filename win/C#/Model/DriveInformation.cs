/*  DriveInformation.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Model
{
    /// <summary>
    /// Information about a DVD drive
    /// </summary>
    public class DriveInformation
    {
        /// <summary>
        /// Gets or sets The Drive Volume Name
        /// </summary>
        public string VolumeLabel { get; set; }

        /// <summary>
        /// Gets or sets The Root Directory
        /// </summary>
        public string RootDirectory { get; set; }
    }
}
