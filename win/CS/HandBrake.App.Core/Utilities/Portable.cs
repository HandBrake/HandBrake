// --------------------------------------------------------------------------------------------------------------------
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

        public static bool IsPortable()
        {
            if (!File.Exists(portableFile))
            {
                return false;
            }

            return true;
        }

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
            return GetBooleanValue("update.check", true);
        }

        public static bool IsHardwareEnabled()
        {
            return GetBooleanValue("hardware.enabled", true);
        }

        public static bool IsProcessIsolationEnabled()
        { 
            return GetBooleanValue("process.isolation.enabled", true);
        }
        
        public static bool IsForcingSoftwareRendering()
        {
            return GetBooleanValue("software.rendering", false);
        }

        public static bool IsThemeEnabled()
        {
            return GetBooleanValue("theme.enabled", true);
        }

        public static bool IsSystemProxyDisabled()
        {
            return GetBooleanValue("force.disable.system.proxy", true);
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

        private static bool GetBooleanValue(string key, bool defaultValue)
        {
            if (keyPairs.ContainsKey(key))
            {
                string value = keyPairs[key];
                if (!string.IsNullOrEmpty(value) && value.Trim() == "true")
                {
                    return true;
                }

                return false;
            }

            return defaultValue;
        }
    }
}