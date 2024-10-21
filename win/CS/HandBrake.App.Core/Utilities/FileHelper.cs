// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FileHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper methods for dealing with files.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;

    /// <summary>
    /// Helper methods for dealing with files.
    /// </summary>
    public class FileHelper
    {
        public static List<string> FileList(string path, bool recursive, List<string> excludedExtensions)
        {
            List<string> foundFiles = new List<string>();

            try
            {
                foreach (string f in Directory.GetFiles(path))
                {
                    string extension = Path.GetExtension(f).Replace(".", string.Empty);
                    if (!excludedExtensions.Contains(extension, StringComparer.OrdinalIgnoreCase))
                    {
                        foundFiles.Add(f);
                    }
                }

                foreach (string d in Directory.GetDirectories(path))
                {
                    foundFiles.AddRange(FileList(d, recursive, excludedExtensions));
                }
            }
            catch (Exception e)
            {
                // Silently ignore. Build up what we can.
                Debug.WriteLine(e);
            }

            return foundFiles;
        }

        public static bool IsDvdOrBluray(string path)
        {
            if (path.Contains("VIDEO_TS", StringComparison.InvariantCultureIgnoreCase) || path.Contains("BDMV", StringComparison.InvariantCultureIgnoreCase))
            {
                return true;
            }

            if (Directory.Exists(Path.Combine(path, "VIDEO_TS")))
            {
                return true;
            }

            if (Directory.Exists(Path.Combine(path, "BDMV")))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// The file path has invalid chars.
        /// </summary>
        /// <param name="path">
        /// The path.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static bool FilePathHasInvalidChars(string path)
        {
            bool result = false;
            if (!string.IsNullOrEmpty(path))
            {
                try
                {
                    string file = Path.GetFileNameWithoutExtension(path);
                    string directory = Path.GetDirectoryName(path);

                    if (path.Split(':').Length - 1 > 1)
                    {
                        return true;
                    }

                    if (!string.IsNullOrEmpty(file) && file.Replace("\"", string.Empty).IndexOfAny(Path.GetInvalidPathChars()) != -1)
                    {
                        return true;
                    }

                    if (!string.IsNullOrEmpty(directory) && directory.Replace("\"", string.Empty).IndexOfAny(Path.GetInvalidPathChars()) != -1)
                    {
                        return true;
                    }
                }
                catch (ArgumentException)
                {
                    result = true;
                }
            }

            return result;
        }
    }
}
