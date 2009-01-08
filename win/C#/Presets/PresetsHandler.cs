using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.IO;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Xml.Serialization;

namespace Handbrake.Presets
{
    public class PresetsHandler
    {
        List<Preset> presets = new List<Preset>();  // Category+Level+Preset Name: Query
        List<Preset> user_presets = new List<Preset>(); // Preset Name: Query
        private static XmlSerializer ser = new XmlSerializer(typeof(List<Preset>));

        /// <summary>
        /// Add a new preset to the system
        /// </summary>
        /// <param name="presetName">String, The name of the new preset</param>
        /// <param name="query">String, the CLI query for the new preset</param>
        /// <param name="pictureSettings"> Bool, store crop/picture sizes in the presets</param>
        public Boolean addPreset(string presetName, string query, Boolean pictureSettings)
        {
            if (checkIfPresetExists(presetName) == false)
            {
                Preset newPreset = new Preset();
                newPreset.Name = presetName;
                newPreset.Query = query;
                newPreset.PictureSettings = pictureSettings;
                user_presets.Add(newPreset);
                updateUserPresetsFile();
                return true;
            }
            else
            {
                MessageBox.Show("Sorry, that preset name already exists. Please choose another!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return false;
            }
        }

        /// <summary>
        /// Remove a preset with a given name from either the built in or user preset list.
        /// </summary>
        /// <param name="name">String, the preset name</param>
        public void remove(string name)
        {
            List<Preset> newPresets = new List<Preset>();
            List<Preset> newUserPresets = new List<Preset>();

            // Built In Presets
            foreach (Preset item in presets)
            {
                if (item.Name != name)
                {
                    newPresets.Add(item);
                }
            }
            presets = newPresets;

            // User Presets
            foreach (Preset item in user_presets)
            {
                if (item.Name != name)
                {
                    newUserPresets.Add(item);
                }
            }
            user_presets = newUserPresets;

            // Rebuild the user_presets.xml file
            updateUserPresetsFile();
            updatePresetsFile();
        }

        /// <summary>
        /// Save changes to a given preset in the user preset list.
        /// </summary>
        /// <param name="presetName">String, The name of the new preset</param>
        /// <param name="query">String, the CLI query for the new preset</param>
        /// <param name="pictureSettings"> Bool, store crop/picture sizes in the preset</param>
        public void updatePreset(string presetName, string query, Boolean pictureSettings)
        {
            // User Presets
            foreach (Preset item in user_presets)
            {
                if (item.Name == presetName)
                {
                    item.Query = query;
                    item.PictureSettings = pictureSettings;
                    MessageBox.Show("Changes to \"" + presetName + "\" Saved", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
        }

        /// <summary>
        /// Get a List of all the built in preset names.
        /// </summary>
        /// <returns>List<String> of preset names</returns>
        public List<Preset> getBuildInPresets()
        {
            return presets;
        }

        /// <summary>
        /// Get a List of all the User preset names.
        /// </summary>
        /// <returns>List<String> of preset names</returns>
        public List<string> getUserPresetNames()
        {
            List<string> names = new List<string>();

            // User Presets
            foreach (Preset item in user_presets)
            {
                names.Add(item.Name);
            }

            return names;
        }

        /// <summary>
        /// Return the CLI query for a preset name given in name
        /// </summary>
        /// <param name="name">String, The preset's name</param>
        /// <returns>String, the CLI query for the given preset name</returns>
        public Preset getPreset(string name)
        {
            // Built In Presets
            foreach (Preset item in presets)
            {
                if (item.Name == name)
                    return item;
            }

            // User Presets
            foreach (Preset item in user_presets)
            {
                if (item.Name == name)
                    return item;
            }

            return null;
        }

        /// <summary>
        /// Reads the CLI's CLI output format and load's them into the preset List<Preset>
        /// </summary>
        public void updateBuiltInPresets()
        {
            // Create a new tempory file and execute the CLI to get the built in presets.
            string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
            string presetsPath = Path.Combine(Path.GetTempPath(), "temp_presets.dat");

            string strCmdLine = String.Format(@"cmd /c """"{0}"" --preset-list >""{1}"" 2>&1""", handbrakeCLIPath, presetsPath);

            ProcessStartInfo hbGetPresets = new ProcessStartInfo("CMD.exe", strCmdLine);
            hbGetPresets.WindowStyle = ProcessWindowStyle.Hidden;

            Process hbproc = Process.Start(hbGetPresets);
            hbproc.WaitForExit();
            hbproc.Dispose();
            hbproc.Close();

            // Clear the current built in presets and now parse the tempory presets file.
            presets.Clear();
            string filePath = Path.Combine(Path.GetTempPath(), "temp_presets.dat");
            if (File.Exists(filePath))
            {
                StreamReader presetInput = new StreamReader(filePath);
                int level = 1;
                string category = String.Empty;
                string level_1_category = String.Empty;

                while (!presetInput.EndOfStream)
                {
                    string line = presetInput.ReadLine();
                    if (line.Contains("<") && !line.Contains("<<"))
                    {
                        level = 1;
                        category = line.Replace("<", "").Trim();
                        level_1_category = category;
                    }

                    if (line.Contains("<<"))
                    {
                        level = 2;
                        category = line.Replace("<<", "").Trim();
                    }

                    if (line.Trim().Contains(">>"))
                    {
                        level = 1;
                        category = level_1_category;
                    }

                    if (line.Contains("+"))
                    {
                        Regex r = new Regex("(:  )"); // Split on hyphens. 
                        string[] presetName = r.Split(line);

                        Preset newPreset = new Preset();
                        newPreset.Level = level;
                        newPreset.Category = category;
                        newPreset.Name = presetName[0].Replace("+", "").Trim();
                        newPreset.Query = presetName[2];
                        presets.Add(newPreset);
                    }
                }
                presetInput.Close();
                presetInput.Dispose();
            }

            // Finally, Create a new or update the current presets.xml file
            updatePresetsFile();
        }

        /// <summary>
        /// Load in the preset data from presets.xml and user_presets.xml
        /// Load it into the 2 arraylist's presets and user_presets
        /// </summary>
        public void loadPresetData()
        {
            // First clear the presets arraylists
            presets.Clear();
            user_presets.Clear();

            string filePath = string.Empty;

            // Load in the users presets from user_presets.xml
            filePath = Application.StartupPath.ToString() + "\\presets.xml";
            if (File.Exists(filePath))
            {
                using (FileStream strm = new FileStream(filePath, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<Preset> list = ser.Deserialize(strm) as List<Preset>;

                        foreach (Preset preset in list)
                            presets.Add(preset);
                    }
                }
            }

            // Load in the users presets from user_presets.xml
            filePath = Application.StartupPath.ToString() + "\\user_presets.xml";
            if (File.Exists(filePath))
            {
                using (FileStream strm = new FileStream(filePath, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<Preset> list = ser.Deserialize(strm) as List<Preset>;

                        foreach (Preset preset in list)
                            user_presets.Add(preset);
                    }
                }
            }
        }

        /// <summary>
        /// Updates the presets.xml file which contains the built in presets
        /// It takes the List of Presets and converts them into XML which is stored in presets.xml
        /// </summary>
        private void updatePresetsFile()
        {
            string userPresets = Application.StartupPath.ToString() + "\\presets.xml";
            try
            {
                using (FileStream strm = new FileStream(userPresets, FileMode.Create, FileAccess.Write))
                {
                    ser.Serialize(strm, presets);
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n Error Information: \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
        }

        /// <summary>
        /// Updates the user_presets.xml file which contains the built in presets
        /// It takes the List of Presets and converts them into XML which is stored in user_presets.xml
        /// </summary>
        private void updateUserPresetsFile()
        {
            string userPresets = Application.StartupPath.ToString() + "\\user_presets.xml";
            try
            {
                using (FileStream strm = new FileStream(userPresets, FileMode.Create, FileAccess.Write))
                {
                    ser.Serialize(strm, user_presets);
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n Error Information: \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
        }

        /// <summary>
        /// Check if the preset "name" exists in either presets or user_presets lists.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        private Boolean checkIfPresetExists(string name)
        {
            if (name == string.Empty)
                return true;

            // Built In Presets
            foreach (Preset item in presets)
            {
                if (item.Name == name)
                    return true;
            }

            // User Presets
            foreach (Preset item in user_presets)
            {
                if (item.Name == name)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Check if the user preset "name" exists in user_presets list.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public Boolean checkIfUserPresetExists(string name)
        {
            if (name == string.Empty)
                return false;

            // User Presets
            foreach (Preset item in user_presets)
            {
                if (item.Name == name)
                    return true;
            }

            return false;
        }
    }
}