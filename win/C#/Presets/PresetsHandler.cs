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
        List<Preset> _presets = new List<Preset>();
        List<Preset> _userPresets = new List<Preset>();
        private static readonly XmlSerializer Ser = new XmlSerializer(typeof(List<Preset>));
        readonly string _userPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\user_presets.xml";
        readonly string _hbPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\presets.xml";

        /// <summary>
        /// Add a new preset to the system
        /// </summary>
        /// <param name="presetName">String, The name of the new preset</param>
        /// <param name="query">String, the CLI query for the new preset</param>
        /// <param name="pictureSettings"> Bool, store crop/picture sizes in the _presets</param>
        public Boolean Add(string presetName, string query, Boolean pictureSettings)
        {
            if (CheckIfPresetExists(presetName) == false)
            {
                Preset newPreset = new Preset { Name = presetName, Query = query, PictureSettings = pictureSettings, Version = Properties.Settings.Default.hb_version };
                _userPresets.Add(newPreset);
                UpdatePresetFiles();
                return true;
            }
            return false;
        }

        /// <summary>
        /// Remove a preset with a given name from either the built in or user preset list.
        /// </summary>
        /// <param name="name">String, the preset name</param>
        public void Remove(string name)
        {
            List<Preset> newPresets = new List<Preset>();
            List<Preset> newUserPresets = new List<Preset>();

            // Built In Presets
            foreach (Preset item in _presets)
            {
                if (item.Name != name)
                {
                    newPresets.Add(item);
                }
            }
            _presets = newPresets;

            // User Presets
            foreach (Preset item in _userPresets)
            {
                if (item.Name != name)
                {
                    newUserPresets.Add(item);
                }
            }
            _userPresets = newUserPresets;

            // Rebuild the _userPresets.xml file
            UpdatePresetFiles();
            UpdatePresetFiles();
        }

        /// <summary>
        /// Remove all built in _presets;
        /// </summary>
        public void RemoveBuiltInPresets()
        {
            _presets.Clear();
            UpdatePresetFiles();
        }

        /// <summary>
        /// Save changes to a given preset in the user preset list.
        /// </summary>
        /// <param name="presetName">String, The name of the new preset</param>
        /// <param name="query">String, the CLI query for the new preset</param>
        /// <param name="pictureSettings"> Bool, store crop/picture sizes in the preset</param>
        public void Update(string presetName, string query, Boolean pictureSettings)
        {
            // User Presets
            foreach (Preset item in _userPresets)
            {
                if (item.Name == presetName)
                {
                    item.Query = query;
                    item.PictureSettings = pictureSettings;
                    MessageBox.Show("Changes to \"" + presetName + "\" Saved", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    UpdatePresetFiles();
                }
            }
        }

        /// <summary>
        /// Return the CLI query for a preset name given in name
        /// </summary>
        /// <param name="name">String, The preset's name</param>
        /// <returns>String, the CLI query for the given preset name</returns>    not
        public Preset GetPreset(string name)
        {
            // Built In Presets
            foreach (Preset item in _presets)
            {
                if (item.Name == name)
                    return item;
            }

            // User Presets
            foreach (Preset item in _userPresets)
            {
                if (item.Name == name)
                    return item;
            }

            return null;
        }

        /// <summary>
        /// Reads the CLI's CLI output format and load's them into the preset List<Preset>
        /// </summary>
        public void UpdateBuiltInPresets()
        {
            // Create a new tempory file and execute the CLI to get the built in _presets.
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

            // Clear the current built in _presets and now parse the tempory _presets file.
            _presets.Clear();

            if (File.Exists(presetsPath))
            {
                StreamReader presetInput = new StreamReader(presetsPath);

                string category = String.Empty;

                while (!presetInput.EndOfStream)
                {
                    string line = presetInput.ReadLine();
                    if (line.Contains("<") && !line.Contains("<<")) // Found the beginning of a preset block 
                        category = line.Replace("<", "").Trim();

                    if (line.Contains("+")) // A Preset
                    {
                        Regex r = new Regex("(:  )"); // Split on hyphens. 
                        string[] presetName = r.Split(line);

                        Preset newPreset = new Preset
                                               {   Category = category,
                                                   Name = presetName[0].Replace("+", "").Trim(),
                                                   Query = presetName[2],
                                                   Version = Properties.Settings.Default.hb_version,
                                                   PictureSettings = true
                                               };
                        _presets.Add(newPreset);
                    }
                }
                presetInput.Close();
                presetInput.Dispose();
            }

            // Finally, Create a new or update the current _presets.xml file
            UpdatePresetFiles();
        }

        /// <summary>
        /// Load in the preset data from _presets.xml and _userPresets.xml
        /// Load it into the 2 arraylist's _presets and _userPresets
        /// </summary>
        private void LoadPresetData()
        {
            // First clear the _presets arraylists
            _presets.Clear();
            _userPresets.Clear();

            // Load in the users _presets from _userPresets.xml
            if (File.Exists(_hbPresetFile))
            {
                using (FileStream strm = new FileStream(_hbPresetFile, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<Preset> list = Ser.Deserialize(strm) as List<Preset>;

                        if (list != null)
                            foreach (Preset preset in list)
                                _presets.Add(preset);
                    }
                }
            }

            // Load in the users _presets from _userPresets.xml
            if (File.Exists(_userPresetFile))
            {
                using (FileStream strm = new FileStream(_userPresetFile, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        List<Preset> list = Ser.Deserialize(strm) as List<Preset>;

                        if (list != null)
                            foreach (Preset preset in list)
                                _userPresets.Add(preset);
                    }
                }
            }
        }

        /// <summary>
        /// Setup the frmMain preset panel
        /// </summary>
        /// <param name="presetPanel"></param>
        public void GetPresetPanel(ref TreeView presetPanel)
        {
            this.LoadPresetData();
            presetPanel.Nodes.Clear();

            if (_presets.Count != 0) // Built In Presets
            {
                string category = string.Empty;
                TreeNode rootNode = null;

                foreach (Preset preset in _presets)
                {
                    if (preset.Category != category)
                    {
                        rootNode = new TreeNode(preset.Category);
                        presetPanel.Nodes.Add(rootNode);
                        category = preset.Category;
                    }

                    if (preset.Category == category && rootNode != null)
                        rootNode.Nodes.Add(preset.Name);
                }
            }

            foreach (Preset preset in _userPresets) // User Presets
            {
                TreeNode presetTreeview = new TreeNode(preset.Name) { ForeColor = Color.Black };
                presetPanel.Nodes.Add(presetTreeview);
            }
        }

        /// <summary>
        /// Update the preset files
        /// </summary>
        private void UpdatePresetFiles()
        {
            try
            {
                using (FileStream strm = new FileStream(_hbPresetFile, FileMode.Create, FileAccess.Write))
                {
                    Ser.Serialize(strm, _presets);
                    strm.Close();
                    strm.Dispose();
                }

                using (FileStream strm = new FileStream(_userPresetFile, FileMode.Create, FileAccess.Write))
                {
                    Ser.Serialize(strm, _userPresets);
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
        /// Check if the preset "name" exists in either _presets or _userPresets lists.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        private Boolean CheckIfPresetExists(string name)
        {
            if (name == string.Empty)
                return true;

            // Built In Presets
            foreach (Preset item in _presets)
            {
                if (item.Name == name)
                    return true;
            }

            // User Presets
            foreach (Preset item in _userPresets)
            {
                if (item.Name == name)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Check if the user preset "name" exists in _userPresets list.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public Boolean CheckIfUserPresetExists(string name)
        {
            if (name == string.Empty)
                return false;

            // User Presets
            foreach (Preset item in _userPresets)
            {
                if (item.Name == name)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Check if the built in _presets stored are not out of date.
        /// Update them if they are.
        /// </summary>
        /// <returns></returns>
        public Boolean CheckIfPresetsAreOutOfDate()
        {
            LoadPresetData();
            // Update built-in _presets if the built-in _presets belong to an older version.
            if (_presets.Count != 0)
                if (_presets[0].Version != Properties.Settings.Default.hb_version)
                {
                    UpdateBuiltInPresets();
                    return true;
                }

            return false;
        }
    }
}