/*  DriveInformation.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Model
{
    public class DriveInformation
    {
        /// <summary>
        /// The Drive Volume Name
        /// </summary>
        public string VolumeLabel { get; set; }

        /// <summary>
        /// The Root Directory
        /// </summary>
        public string RootDirectory { get; set; }
    }
}
