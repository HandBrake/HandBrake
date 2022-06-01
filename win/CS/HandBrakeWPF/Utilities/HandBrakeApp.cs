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

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Utilities;

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
            string appDataFolder = DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly());
            DeleteFile(Path.Combine(appDataFolder, "presets.json"));
            DeleteFile(Path.Combine(appDataFolder, "settings.json"));

            DirectoryInfo info = new DirectoryInfo(appDataFolder);
            IEnumerable<FileInfo> logFiles = info.GetFiles("*.json").Where(f => f.Name.StartsWith("hb_queue_recovery"));
            foreach (FileInfo file in logFiles)
            {
                DeleteFile(Path.Combine(appDataFolder, file.Name));
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
            catch (Exception)
            {
                throw;
            }
        }
    }
}
