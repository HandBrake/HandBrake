// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeApp.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A general Helper class for HandBrake GUI
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;

    /// <summary>
    /// A general Helper class for HandBrake GUI
    /// </summary>
    public class HandBrakeApp
    {
        /// <summary>
        /// The reset to defaults.
        /// </summary>
        public static void ResetToDefaults()
        {
            DeleteFile(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\presets.xml");
            DeleteFile(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\user_presets.xml");
            DeleteFile(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\settings.xml");

            string tempPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            DirectoryInfo info = new DirectoryInfo(tempPath);
            IEnumerable<FileInfo> logFiles = info.GetFiles("*.xml").Where(f => f.Name.StartsWith("hb_queue_recovery"));
            foreach (FileInfo file in logFiles)
            {
                DeleteFile(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\" + file.Name);
            }
        }

        /// <summary>
        /// The delete file.
        /// </summary>
        /// <param name="file">
        /// The file.
        /// </param>
        private static void DeleteFile(string file)
        {
            try
            {
                if (File.Exists(file))
                {
                    File.Delete(file);
                }
            }
            catch (Exception exc)
            {
                throw;
            }
        }
    }
}
