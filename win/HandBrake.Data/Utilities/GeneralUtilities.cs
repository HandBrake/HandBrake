// --------------------------------------------------------------------------------------------------------------------
// <copyright file="GeneralUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Set of Static Utilites
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;
    using HandBrake;
    using HandBrake.CoreLibrary.Utilities;

    /// <summary>
    /// A Set of Static Utilites
    /// </summary>
    public class GeneralUtilities
    {
        #region Constants and Fields

        /// <summary>
        /// The Default Log Directory
        /// </summary>
        private static readonly string LogDir = DirectoryUtilities.GetLogDirectory();

        #endregion Constants and Fields

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

        #endregion Properties

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
                    try
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
                    catch (Exception)
                    {
                        // Silently ignore files we can't delete. They are probably being used by the app right now.
                    }
                }
            }
        }

        /// <summary>
        /// Generate the header for the log file.
        /// </summary>
        /// <returns>
        /// The generatedlog header.
        /// </returns>
        public static StringBuilder CreateLogHeader()
        {
            var logHeader = new StringBuilder();
            var systemInfo = HandBrakeServices.Current.SystemInfo;

            StringBuilder gpuBuilder = new StringBuilder();
            foreach (var item in systemInfo.GPUInfo)
            {
                gpuBuilder.AppendLine(string.Format("  {0}", item));
            }

            if (string.IsNullOrEmpty(gpuBuilder.ToString().Trim()))
            {
                gpuBuilder.Append("GPU Information is unavailable");
            }

            logHeader.AppendLine(string.Format("HandBrake {0} - {1}", VersionHelper.GetVersion(), VersionHelper.GetPlatformBitnessVersion()));
            logHeader.AppendLine(string.Format("OS: {0} - {1}", Environment.OSVersion, Environment.Is64BitOperatingSystem ? "64bit" : "32bit"));
            logHeader.AppendLine(string.Format("CPU: {0}", systemInfo.CPUInformation));
            logHeader.AppendLine(string.Format("Ram: {0} MB, ", systemInfo.TotalPhysicalMemory));
            logHeader.AppendLine(string.Format("GPU Information:{0}{1}", Environment.NewLine, gpuBuilder.ToString().TrimEnd()));
            logHeader.AppendLine(string.Format("Screen: {0}x{1}", systemInfo.ScreenBounds.Width, systemInfo.ScreenBounds.Height));
            logHeader.AppendLine(string.Format("Temp Dir: {0}", Path.GetTempPath()));
            logHeader.AppendLine(string.Format("Install Dir: {0}", systemInfo.InstallLocation));
            logHeader.AppendLine(string.Format("Data Dir: {0}\n", systemInfo.AppDataLocation));

            logHeader.AppendLine("-------------------------------------------");

            return logHeader;
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

        #endregion Public Methods
    }
}