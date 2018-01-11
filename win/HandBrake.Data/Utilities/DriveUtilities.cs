// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DriveUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The drive utilities.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using System.Collections.Generic;
    using System.IO;

    using HandBrake.Model;

    /// <summary>
    /// The drive utilities.
    /// </summary>
    public class DriveUtilities
    {
        /// <summary>
        /// Get a list of available DVD drives which are ready and contain DVD content.
        /// </summary>
        /// <returns>A List of Drives with their details</returns>
        public static List<DriveInformation> GetDrives()
        {
            var drives = new List<DriveInformation>();
            DriveInfo[] theCollectionOfDrives = DriveInfo.GetDrives();
            int id = 0;
            foreach (DriveInfo curDrive in theCollectionOfDrives)
            {
                if (curDrive.DriveType == DriveType.CDRom && curDrive.IsReady)
                {
                    if (Directory.Exists(curDrive.RootDirectory + "VIDEO_TS") ||
                        Directory.Exists(curDrive.RootDirectory + "BDMV"))
                    {
                        // Protect UWP from exceptions arising from Permissions issues.
                        try
                        {
                            drives.Add(
                                new DriveInformation
                                {
                                    Id = id,
                                    VolumeLabel = curDrive.VolumeLabel,
                                    RootDirectory = curDrive.RootDirectory.ToString(),
                                });
                            id++;
                        }
                        catch { }
                    }
                }
            }

            return drives;
        }

        public static bool HasMinimumDiskSpace(string destination, long minimumInBytes)
        {
            // UWP does not permit free space info on System Drives, catch and return true.
            try
            {
                string drive = Path.GetPathRoot(destination);
                if (!string.IsNullOrEmpty(drive) && !drive.StartsWith("\\"))
                {
                    DriveInfo c = new DriveInfo(drive);
                    if (c.AvailableFreeSpace < minimumInBytes)
                    {
                        return false;
                    }
                }
            }
            catch
            {
            }

            return true;
        }
    }
}