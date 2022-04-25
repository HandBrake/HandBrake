﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Portable.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Portable type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Utilities
{
    using System;
    using System.Collections.Generic;
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
        public static int Initialise()
        {
            if (!IsPortable())
            {
                return 0;
            }

            // Read the INI file     
            if (File.Exists(portableFile))
            {
                try
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
                catch
                {
                    return -1;
                }
            }

            // Create any missing directories
            string tmpDir = GetTempDirectory();
            if (!string.IsNullOrEmpty(tmpDir) && !Directory.Exists(tmpDir))
            {
                try
                {
                    Directory.CreateDirectory(tmpDir);
                }
                catch (Exception)
                {
                    return -2;
                }
            }

            string stroageDir = GetStorageDirectory();
            if (!Directory.Exists(stroageDir))
            {
                try
                {
                    Directory.CreateDirectory(stroageDir);
                }
                catch (Exception)
                {
                    return -3;
                }
            }

            // Setup environment variables for this instance.
            if (!string.IsNullOrEmpty(tmpDir))
            {
                Environment.SetEnvironmentVariable("TMP", GetTempDirectory());
            }

            return 0;
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
            // Default to App Data
            string storagePath = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            if (keyPairs.ContainsKey("storage.dir"))
            {
                string directory = keyPairs["storage.dir"];

                // If "cwd", then treat that as Current Working Directory.
                if (!string.IsNullOrEmpty(directory) && directory == "cwd")
                {
                    storagePath = Path.Combine(Environment.CurrentDirectory, "storage");
                }

                // Otherwise, the users set directory.
                if (!string.IsNullOrEmpty(directory) && directory != "cwd")
                {
                    storagePath = directory;
                }
            } 

            // Return what path we figured out to use.
            return storagePath;
        }

        public static bool IsUpdateCheckEnabled()
        {
            if (keyPairs.ContainsKey("update.check"))
            {
                string updateCheckEnabled = keyPairs["update.check"];
                if (!string.IsNullOrEmpty(updateCheckEnabled) && updateCheckEnabled.Trim() == "true")
                {
                    return true;
                }

                return false;
            }

            return true;
        }

        public static bool IsHardwareEnabled()
        {
            if (keyPairs.ContainsKey("hardware.enabled"))
            {
                string hardwareEnabled = keyPairs["hardware.enabled"];
                if (!string.IsNullOrEmpty(hardwareEnabled) && hardwareEnabled.Trim() == "true")
                {
                    return true;
                }

                return false;
            }

            return true; // Default to On.
        }

        public static bool IsProcessIsolationEnabled()
        {
            if (keyPairs.ContainsKey("process.isolation.enabled"))
            {
                string enabled = keyPairs["process.isolation.enabled"];
                if (!string.IsNullOrEmpty(enabled) && enabled.Trim() == "true")
                {
                    return true;
                }

                return false;
            }

            return true; // Default to On.
        }
        
        public static bool IsForcingSoftwareRendering()
        {
            if (keyPairs.ContainsKey("software.rendering"))
            {
                string enabled = keyPairs["software.rendering"];
                if (!string.IsNullOrEmpty(enabled) && enabled.Trim() == "true")
                {
                    return true;
                }

                return false;
            }

            return false; // Default to Off.
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

                if (!string.IsNullOrEmpty(directory) && directory != "cwd")
                {
                    return directory;
                }

                return null;
            }

            return null;
        }
    }
}