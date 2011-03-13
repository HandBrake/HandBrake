/*  UtilityService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// A Set of Static Utilites
    /// </summary>
    public class UtilityService
    {
        /// <summary>
        /// The Default Log Directory
        /// </summary>
        private static readonly string LogDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";

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
                DirectoryInfo info = new DirectoryInfo(LogDir);
                FileInfo[] logFiles = info.GetFiles("*.txt");

                // Delete Them
                foreach (FileInfo file in logFiles)
                {
                    if (file.LastWriteTime < DateTime.Now.AddDays(-daysToKeep))
                    {
                        if (!file.Name.Contains("last_scan_log.txt") && !file.Name.Contains("last_encode_log.txt"))
                        {
                            File.Delete(file.FullName);
                        }
                        else if (file.Length > 104857600)
                        {
                            File.Delete(file.FullName);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Get a list of available DVD drives which are ready and contain DVD content.
        /// </summary>
        /// <returns>A List of Drives with their details</returns>
        public static List<DriveInformation> GetDrives()
        {
            List<DriveInformation> drives = new List<DriveInformation>();
            DriveInfo[] theCollectionOfDrives = DriveInfo.GetDrives();
            int id = 0;
            foreach (DriveInfo curDrive in theCollectionOfDrives)
            {
                if (curDrive.DriveType == DriveType.CDRom && curDrive.IsReady)
                {
                    if (Directory.Exists(curDrive.RootDirectory + "VIDEO_TS") || Directory.Exists(curDrive.RootDirectory + "BDMV"))
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
        /// Get the Process ID of HandBrakeCLI for the current instance.
        /// </summary>
        /// <returns>A list of processes</returns>
        public static Process[] GetCliProcess()
        {
            return Process.GetProcessesByName("HandBrakeCLI");
        }

        /// <summary>
        /// Add the CLI Query to the Log File.
        /// </summary>
        /// <param name="encJob">
        /// The Encode Job Object
        /// </param>
        /// <returns>
        /// The create cli log header.
        /// </returns>
        public static string CreateCliLogHeader(QueueTask encJob)
        {
            StringBuilder logHeader = new StringBuilder();

            logHeader.AppendLine(String.Format("# {0}", Init.HandBrakeGuiVersionString));
            logHeader.AppendLine(String.Format("# Running: {0}", Environment.OSVersion));
            logHeader.AppendLine(String.Format("# CPU: {0}", SystemInfo.GetCpuCount));
            logHeader.AppendLine(String.Format("# Ram: {0} MB", SystemInfo.TotalPhysicalMemory));
            logHeader.AppendLine(String.Format("# Screen: {0}x{1}", SystemInfo.ScreenBounds.Bounds.Width, SystemInfo.ScreenBounds.Bounds.Height));
            logHeader.AppendLine(String.Format("# Temp Dir: {0}", Path.GetTempPath()));
            logHeader.AppendLine(String.Format("# Install Dir: {0}", Application.StartupPath));
            logHeader.AppendLine(String.Format("# Data Dir: {0}\n", Application.UserAppDataPath));

            if (encJob != null)
            {
                logHeader.AppendLine(String.Format("# CLI Query: {0}", encJob.Query));
                logHeader.AppendLine(String.Format("# User Query: {0}", encJob.CustomQuery));
            }
            logHeader.AppendLine("-------------------------------------------");

            return logHeader.ToString();
        }
    }
}
