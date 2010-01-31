/*  SourceType.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Model
{
    public enum SourceType
    {
        /// <summary>
        /// No Source Selected
        /// </summary>
        None = 0,

        /// <summary>
        /// The soruce selected is a folder
        /// </summary>
        Folder,

        /// <summary>
        /// The source selected is a DVD drive
        /// </summary>
        DvdDrive,

        /// <summary>
        /// The source selected is a Video File
        /// </summary>
        VideoFile
    }
}
