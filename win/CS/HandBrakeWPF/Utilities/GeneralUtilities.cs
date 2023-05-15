// --------------------------------------------------------------------------------------------------------------------
// <copyright file="GeneralUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Set of Static Utilities
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

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;

    /// <summary>
    /// A Set of Static Utilities
    /// </summary>
    public class GeneralUtilities
    {
        private static readonly string LogDir = DirectoryUtilities.GetLogDirectory();

        public static int ProcessId
        {
            get
            {
                return Process.GetCurrentProcess().Id;
            }
        }

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

                // Delete old and excessively large files (> ~50MB).
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

            StringBuilder gpuBuilder = new StringBuilder();
            foreach (var item in SystemInfo.GetGPUInfo)
            {
                gpuBuilder.AppendLine(string.Format("  {0}", item.DisplayValue));
            }

            if (string.IsNullOrEmpty(gpuBuilder.ToString().Trim()))
            {
                gpuBuilder.Append("GPU Information is unavailable");
            }

            logHeader.AppendLine(string.Format("HandBrake {0}", HandBrakeVersionHelper.GetVersion()));
            logHeader.AppendLine(string.Format("OS: {0}", Environment.OSVersion));
            logHeader.AppendLine(string.Format("CPU: {0}", SystemInfo.GetCpu));
            logHeader.AppendLine(string.Format("Ram: {0} MB, ", SystemInfo.TotalPhysicalMemory));
            logHeader.AppendLine(string.Format("GPU Information:{0}{1}", Environment.NewLine, gpuBuilder.ToString().TrimEnd()));
            logHeader.AppendLine(string.Format("Screen: {0}", SystemInfo.ScreenBounds));
            logHeader.AppendLine(string.Format("Temp Dir: {0}", Path.GetTempPath()));
            logHeader.AppendLine(string.Format("Install Dir: {0}", AppDomain.CurrentDomain.BaseDirectory));
            logHeader.AppendLine(string.Format("Data Dir: {0}\n", DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly())));

            logHeader.Append("-------------------------------------------");

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
    }
}