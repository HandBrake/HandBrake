using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Text.RegularExpressions;


namespace Handbrake.Functions
{
    public class Presets
    {
        List<string> presets = new List<string>();
        List<string> user_presets = new List<string>();

        /// <summary>
        /// Add a new preset to the system
        /// </summary>
        /// <param name="presetName">String, The name of the new preset</param>
        /// <param name="query">String, the CLI query for the new preset</param>
        public Boolean addPreset(string presetName, string query)
        {
            if (checkIfPresetExists(presetName) == false)
            {
                String preset = "+ " + presetName + ":  " + query;
                user_presets.Add(preset);
                addPresetToFile(preset);
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
            List<string> newPresets = new List<string>();
            List<string> newUserPresets = new List<string>();

            // Built In Presets
            foreach (string item in presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )"); // Split on hyphens. 
                string[] presetName = r.Split(x);
                if (presetName[0] != name)
                    newPresets.Add(item);
            }

            // User Presets
            foreach (string item in user_presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )"); // Split on hyphens. 
                string[] presetName = r.Split(x);
                if (presetName[0] != name)
                    newUserPresets.Add(item);
            }

            // Now, Update the presets.dat and user_presets.dat file with the new items.
            string userPresets = Application.StartupPath.ToString() + "\\user_presets.dat";
            string presetsFile = Application.StartupPath.ToString() + "\\presets.dat";

            // Rebuild the presets.dat file
            StreamWriter line = new StreamWriter(presetsFile);
            foreach (string item in newPresets)
            {
                line.WriteLine("+ " + item);
            }
            line.Close();
            line.Dispose();

            // Rebuild the user_presets.dat file
            line = new StreamWriter(userPresets);
            foreach (string item in newUserPresets)
            {
                line.WriteLine("+ " + item);
            }
            line.Close();
            line.Dispose();
        }

        /// <summary>
        /// Get a List of all the built in preset names.
        /// </summary>
        /// <returns>List<String> of preset names</returns>
        public List<string> getBuildInPresetNames()
        {
            List<string> names = new List<string>();

            // Built In Presets
            foreach (string item in presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )"); // Split on hyphens. 
                string[] presetName = r.Split(x);
                names.Add(presetName[0]);

            }
            return names;
        }

        /// <summary>
        /// Get a List of all the User preset names.
        /// </summary>
        /// <returns>List<String> of preset names</returns>
        public List<string> getUserPresetNames()
        {
            List<string> names = new List<string>();

            // User Presets
            foreach (string item in user_presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )");
                string[] presetName = r.Split(x);
                names.Add(presetName[0]);

            }

            return names;
        }

        /// <summary>
        /// Return the CLI query for a preset name given in name
        /// </summary>
        /// <param name="name">String, The preset's name</param>
        /// <returns>String, the CLI query for the given preset name</returns>
        public string getCliForPreset(string name)
        {
            // Built In Presets
            foreach (string item in presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )"); // Split on hyphens. 
                string[] presetName = r.Split(x);
                if (presetName[0] == name)
                    return presetName[2];
            }

            // User Presets
            foreach (string item in user_presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )"); // Split on hyphens. 
                string[] presetName = r.Split(x);
                if (presetName[0] == name)
                    return presetName[2];
            }

            return null;
        }

        /// <summary>
        /// Load in the preset data from presets.dat and user_presets.dat
        /// Load it into the 2 arraylist's presets and user_presets
        /// </summary>
        public void loadPresetFiles()
        {
            // First clear the presets arraylists
            presets.Clear();
            user_presets.Clear();

            // Load in the built in presets from presets.dat
            // We'll store them in the array in the format:   presetName:  ClI Query
            string filePath = Application.StartupPath.ToString() + "\\presets.dat";
            if (File.Exists(filePath))
            {
                StreamReader presetInput = new StreamReader(filePath);
                while (!presetInput.EndOfStream)
                {
                    if ((char)presetInput.Peek() == '+')
                        presets.Add(presetInput.ReadLine().Replace("+ ", ""));
                    else
                        presetInput.ReadLine();
                }
                presetInput.Close();
                presetInput.Dispose();
            }

            // Load in the users presets from user_presets.dat
            filePath = Application.StartupPath.ToString() + "\\user_presets.dat";
            if (File.Exists(filePath))
            {
                StreamReader presetInput = new StreamReader(filePath);
                while (!presetInput.EndOfStream)
                {
                    if ((char)presetInput.Peek() == '+')
                        user_presets.Add(presetInput.ReadLine().Replace("+ ", ""));
                    else
                        presetInput.ReadLine();
                }
                presetInput.Close();
                presetInput.Dispose();
            }
        }

        // Add a single preset to user_presets.dat
        private void addPresetToFile(string preset)
        {
            string userPresets = Application.StartupPath.ToString() + "\\user_presets.dat";
            try
            {
                // Create a StreamWriter and open the file
                StreamWriter line = File.AppendText(userPresets);

                // Generate and write the preset string to the file
                line.WriteLine(preset);

                // close the stream
                line.Close();
                line.Dispose();
            }
            catch (Exception exc)
            {
                MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n Error Information: \n\n" + exc.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
        }

        // Check if a preset already exists in either the built in or user presets
        private Boolean checkIfPresetExists(string name)
        {
            if (name == string.Empty)
                return true;

            // Built In Presets
            foreach (string item in presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )"); // Split on hyphens. 
                string[] presetName = r.Split(x);
                if (presetName[0] == name)
                    return true;
            }

            // User Presets
            foreach (string item in user_presets)
            {
                string x = item.Replace("+ ", "");
                Regex r = new Regex("(:  )"); // Split on hyphens. 
                string[] presetName = r.Split(x);
                if (presetName[0] == name)
                    return true;
            }

            return false;
        }
    }

}