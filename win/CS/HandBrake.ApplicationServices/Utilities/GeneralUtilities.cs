// --------------------------------------------------------------------------------------------------------------------
// <copyright file="GeneralUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Set of Static Utilites
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// A Set of Static Utilites
    /// </summary>
    public class GeneralUtilities
    {
        #region Constants and Fields

        /// <summary>
        /// The Default Log Directory
        /// </summary>
        private static readonly string LogDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";

        #endregion

        #region Properties

        /// <summary>
        /// Gets the number of HandBrake instances running.
        /// </summary>
        public static int ProcessId
        {
            get
            {
                return Process.GetCurrentProcess().Id;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Clear all the log files older than 30 Days
        /// </summary>
        /// <param name="daysToKeep">
        /// The Number of Days to Keep
        /// </param>
        public static void ClearLogFiles(int daysToKeep)
        {
            if (Directory.Exists(LogDir))
            {
                // Get all the log files
                var info = new DirectoryInfo(LogDir);
                FileInfo[] logFiles = info.GetFiles("*.txt");

                // Delete old and excessivly large files (> ~50MB).
                foreach (FileInfo file in logFiles)
                {
                    if (file.LastWriteTime < DateTime.Now.AddDays(-daysToKeep))
                    {
                        File.Delete(file.FullName);
                    }
                    else if (file.Length > 50000000)
                    {
                        File.Delete(file.FullName);
                    }
                }
            }
        }

        /// <summary>
        /// Add the CLI Query to the Log File.
        /// </summary>
        /// <returns>
        /// The create cli log header.
        /// </returns>
        public static StringBuilder CreateCliLogHeader()
        {
            var logHeader = new StringBuilder();

            logHeader.AppendLine(String.Format("HandBrake {0} - {1}", VersionHelper.GetVersion(), VersionHelper.GetPlatformBitnessVersion()));
            logHeader.AppendLine(String.Format("OS: {0}", Environment.OSVersion));
            logHeader.AppendLine(String.Format("CPU: {0}", SystemInfo.GetCpuCount));
            logHeader.Append(String.Format("Ram: {0} MB, ", SystemInfo.TotalPhysicalMemory));
            logHeader.AppendLine(String.Format("Screen: {0}x{1}", SystemInfo.ScreenBounds.Bounds.Width, SystemInfo.ScreenBounds.Bounds.Height));
            logHeader.Append(String.Format("GPU: {0}\n", SystemInfo.GetGPUName));
            logHeader.Append(String.Format("GPU driver version: {0}\n", SystemInfo.GetGPUDriverVersion));
            logHeader.AppendLine(String.Format("Temp Dir: {0}", Path.GetTempPath()));
            logHeader.AppendLine(String.Format("Install Dir: {0}", Application.StartupPath));
            logHeader.AppendLine(String.Format("Data Dir: {0}\n", Application.UserAppDataPath));

            logHeader.AppendLine("-------------------------------------------");

            return logHeader;
        }

        /// <summary>
        /// Get the Process ID of HandBrakeCLI for the current instance.
        /// </summary>
        /// <returns>A list of processes</returns>
        public static Process[] GetCliProcess()
        {
            return Process.GetProcessesByName("HandBrakeCLI");
        }

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
                        drives.Add(
                            new DriveInformation
                                {
                                    Id = id,
                                    VolumeLabel = curDrive.VolumeLabel,
                                    RootDirectory = curDrive.RootDirectory.ToString()
                                });
                        id++;
                    }
                }
            }

            return drives;
        }

        /// <summary>
        /// Return the standard log format line of text for a given log message
        /// </summary>
        /// <param name="message">
        /// The Log Message
        /// </param>
        /// <returns>
        /// A Log Message in the format: "[hh:mm:ss] message"
        /// </returns>
        public static string LogLine(string message)
        {
            return string.Format("[{0}] {1}", DateTime.Now.TimeOfDay, message);
        }

        /// <summary>
        /// The find hand brake instance ids.
        /// </summary>
        /// <param name="id">
        /// The id.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>. True if it's a running HandBrake instance.
        /// </returns>
        public static bool IsPidACurrentHandBrakeInstance(int id)
        {
            List<int> ids = Process.GetProcessesByName("HandBrake").Select(process => process.Id).ToList();
            return ids.Contains(id);
        }

        #endregion
    }
}