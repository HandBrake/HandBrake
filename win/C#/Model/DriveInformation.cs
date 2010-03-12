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
        /// Gets or sets A Unique ID That represemts this model.
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
        /// Returns  "Drive" + Id  (e.g  Drive2)
        /// </summary>
        /// <returns>
        /// A String that contrains "Drive" and it's ID
        /// </returns>
        public override string ToString()
        {
            return "Drive" + Id;
        }
    }
}