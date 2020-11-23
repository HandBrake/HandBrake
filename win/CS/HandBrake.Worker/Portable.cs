// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Portable.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Portable type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker
{
    using System;
    using System.Collections.Generic;
    using System.IO;

    public class Portable
    {
        private static readonly string PortableFile = Path.Combine(Environment.CurrentDirectory, "portable.ini");
        private static Dictionary<string, string> keyPairs = new Dictionary<string, string>();

        public static bool Initialise()
        {
            if (!IsPortable())
            {
                return true;
            }

            // Read the INI file     
            if (File.Exists(PortableFile))
            {
                try
                {
                    using (StreamReader fileReader = new StreamReader(PortableFile))
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
                    return false;
                }
            }

            return true;
        }

        public static bool IsPortable()
        {
            if (!File.Exists(PortableFile))
            {
                return false;
            }

            return true;
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
    }
}