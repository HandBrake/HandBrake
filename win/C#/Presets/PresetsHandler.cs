/*  PresetHandler.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.IO;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Xml.Serialization;

namespace Handbrake.Presets
{
    public class PresetsHandler
    {
        List<Preset> presets = new List<Preset>();
        List<Preset> user_presets = new List<Preset>();
        private static readonly XmlSerializer ser = new XmlSerializer(typeof(List<Preset>));
        String userPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\user_presets.xml";
        string hbPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\presets.xml";

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
                Preset newPreset = new Preset { Name = presetName, Query = query, PictureSettings = pictureSettings, Version = Properties.Settings.Default.hb_version};
                user_presets.Add(newPreset);
                updateUserPresetsFile();
                return true;
            }
            MessageBox.Show("Sorry, that preset name already exists. Please choose another!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return false;
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
                    updateUserPresetsFile();
                }
            }
        }

        /// <summary>
        /// Return the CLI query for a preset name given in name
        /// </summary>
        /// <param name="name">String, The preset's name</param>
        /// <returns>String, the CLI query for the given preset name</returns>    not
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

            ProcessStartInfo hbGetPresets = new ProcessStartInfo("CMD.exe", strCmdLine) { WindowStyle = ProcessWindowStyle.Hidden };
            Process hbproc = Process.Start(hbGetPresets);
            if (hbproc != null)
            {
                hbproc.WaitForExit();
                hbproc.Dispose();
                hbproc.Close();
            }

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
                    if (line.Contains("<") && !line.Contains("<<")) // Found the beginning of a preset block 
                    {
                        level = 1;
                        category = line.Replace("<", "").Trim();
                        level_1_category = category;
                    }

                    if (line.Contains("<<")) // found a sub preset block
                    {
                        level = 2;
                        category = line.Replace("<<", "").Trim();
                    }

                    if (line.Trim().Contains(">>")) // End of sub preset block
                    {
                        level = 1;
                        category = level_1_category;
                    }

                    if (line.Contains("+")) // A Preset
                    {
                        Regex r = new Regex("(:  )"); // Split on hyphens. 
                        string[] presetName = r.Split(line);

                        Preset newPreset = new Preset
                                               {
                                                   Level = level,
                                                   Category = category,
                                                   TopCategory = level_1_category,
                                                   Name = presetName[0].Replace("+", "").Trim(),
                                                   Query = presetName[2],
                                                   Version = Properties.Settings.Default.hb_version
                                               };
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
        private void loadPresetData()
        {
            // First clear the presets arraylists
            presets.Clear();
            user_presets.Clear();

            // Load in the users presets from user_presets.xml
            if (File.Exists(hbPresetFile))
            {
                using (FileStream strm = new FileStream(hbPresetFile, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<Preset> list = ser.Deserialize(strm) as List<Preset>;

                        if (list != null)
                            foreach (Preset preset in list)
                                presets.Add(preset);
                    }
                }
            }

            // Load in the users presets from user_presets.xml
            if (File.Exists(userPresetFile))
            {
                using (FileStream strm = new FileStream(userPresetFile, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<Preset> list = ser.Deserialize(strm) as List<Preset>;

                        if (list != null)
                            foreach (Preset preset in list)
                                user_presets.Add(preset);
                    }
                }
            }
        }

        /// <summary>
        /// Setup the frmMain preset panel
        /// </summary>
        /// <param name="presetPanel"></param>
        public void getPresetPanel(ref TreeView presetPanel)
        {
            this.loadPresetData();
            presetPanel.Nodes.Clear();

            if (presets.Count != 0)
            {
                string category = presets[0].Category;
                TreeNode rootNode = new TreeNode(presets[0].Category);
                TreeNode childNode = null;
                Boolean addChildNode = false;

                foreach (Preset preset in presets)
                {
                    // Deal with Root
                    if (preset.Category == category && preset.TopCategory == category)
                        rootNode.Nodes.Add(preset.Name);
                    else if (preset.Category != category && preset.TopCategory == category) // Deal with child nodes for that root
                    {
                        if (childNode == null) // For the first child node
                            childNode = new TreeNode(preset.Category);

                        childNode.Nodes.Add(preset.Name);
                        addChildNode = true;
                    }
                    else if (preset.Category != category && preset.TopCategory != category && preset.Level == 1)// Deal with changing root nodes
                    {
                        // If we find there are child nodes, add them to the current root node set before adding the rootnode to the panel.
                        if (addChildNode)
                        {
                            rootNode.Nodes.Add(childNode);
                            childNode = null;
                            addChildNode = false;
                        }
                        // Add the current rootnodes to the panel, and prepare for a new category of presets
                        presetPanel.Nodes.Add(rootNode);
                        rootNode = new TreeNode(preset.Category);
                        rootNode.Nodes.Add(preset.Name);

                        category = preset.Category;
                    }
                }
                presetPanel.Nodes.Add(rootNode); // Add the final set of nodes 
            }

            // User Presets
            foreach (Preset preset in user_presets)
            {
                TreeNode preset_treeview = new TreeNode(preset.Name) { ForeColor = Color.Black };
                presetPanel.Nodes.Add(preset_treeview);
            }
        }

        /// <summary>
        /// Updates the presets.xml file which contains the built in presets
        /// It takes the List of Presets and converts them into XML which is stored in presets.xml
        /// </summary>
        private void updatePresetsFile()
        {
            string userPresets = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\presets.xml";
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
                MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n Error Information: \n\n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
        }

        /// <summary>
        /// Updates the user_presets.xml file which contains the built in presets
        /// It takes the List of Presets and converts them into XML which is stored in user_presets.xml
        /// </summary>
        private void updateUserPresetsFile()
        {
            try
            {
                using (FileStream strm = new FileStream(userPresetFile, FileMode.Create, FileAccess.Write))
                {
                    ser.Serialize(strm, user_presets);
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n Error Information: \n\n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
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

        /// <summary>
        /// Check if the built in presets stored are not out of date.
        /// Update them if they are.
        /// </summary>
        /// <returns></returns>
        public Boolean checkIfPresetsAreOutOfDate()
        {
            loadPresetData();
            // Update built-in presets if the built-in presets belong to an older version.
            if (presets.Count != 0)
                if (presets[0].Version != Properties.Settings.Default.hb_version)
                {
                    updateBuiltInPresets();
                    return true;
                }

            return false;
        }
    }
}