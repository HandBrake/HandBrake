// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FileHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper methods for dealing with files.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.IO;

    /// <summary>
    /// Helper methods for dealing with files.
    /// </summary>
    public class FileHelper
    {
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
