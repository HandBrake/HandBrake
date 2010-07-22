/*  Logging.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.IO;
    using System.Text;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The System Information.
    /// </summary>
    public class Logging
    {
        /// <summary>
        /// Add the CLI Query to the Log File.
        /// </summary>
        /// <param name="encJob">
        /// The Encode Job Object
        /// </param>
        /// <returns>String with the log header</returns>
        public static string CreateCliLogHeader(Job encJob)
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