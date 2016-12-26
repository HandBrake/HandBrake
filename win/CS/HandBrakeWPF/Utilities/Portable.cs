// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Portable.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Portable type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.Collections.Generic;
    using System;
    using System.IO;

    /// <summary>
    /// This class is responsible for reading the Portable.ini file that allows HandBrake to be run out of a directory.
    /// </summary>
    public class Portable
    {
        private static readonly string portableFile = Path.Combine(Environment.CurrentDirectory, "portable.ini");
        private static Dictionary<string, string> keyPairs = new Dictionary<string, string>();      

        /// <summary>
        /// Initializes a new instance of the <see cref="Portable"/> class.
        /// </summary>
        public static void Initialise()
        {
            if (!IsPortable())
            {
                return; // Nothing to do.
            }

            // Read the INI file     
            if (File.Exists(portableFile))
            {
                using (StreamReader fileReader = new StreamReader(portableFile))
                {
                    string line;
                    while ((line = fileReader.ReadLine()) != null)
                    {
                        line = line.Trim();

                        if (line.StartsWith("#"))
                        {
                            continue; // Ignore Comments
                        }

                        string[] setting = line.Split('=');
                        if (setting.Length == 2)
                        {
                            keyPairs.Add(setting[0].Trim(), setting[1].Trim());
                        }
                    }
                }
            }

            // Create any missing directories
            if (!Directory.Exists(GetTempDirectory()))
            {
                Directory.CreateDirectory(GetTempDirectory());
            }

            if (!Directory.Exists(GetStorageDirectory()))
            {
                Directory.CreateDirectory(GetStorageDirectory());
            }

            // Setup enviroment variables for this instance.
            Environment.SetEnvironmentVariable("TMP", GetTempDirectory());
        }

        /// <summary>
        /// The is portable.
        /// </summary>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public static bool IsPortable()
        {
            if (!File.Exists(portableFile))
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// The get config directory.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static string GetStorageDirectory()
        {
            if (keyPairs.ContainsKey("storage.dir"))
            {
                string directory = keyPairs["storage.dir"];
                if (directory == "cwd")
                {
                    return Path.Combine(Environment.CurrentDirectory, "storage");
                }

                return directory;
            }

            return null;
        }

        /// <summary>
        /// The get temp directory.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        private static string GetTempDirectory()
        {
            if (keyPairs.ContainsKey("tmp.dir"))
            {
                string directory = keyPairs["tmp.dir"];
                if (directory == "cwd")
                {
                    return Path.Combine(Environment.CurrentDirectory, "tmp");
                }

                return directory;
            }

            return null;
        }
    }
}