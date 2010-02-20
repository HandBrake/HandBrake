/*  PresetHandler.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */
namespace Handbrake.Presets
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.Text.RegularExpressions;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    public class PresetsHandler
    {
        private List<Preset> Presets = new List<Preset>();
        private List<Preset> UserPresets = new List<Preset>();
        private static readonly XmlSerializer Ser = new XmlSerializer(typeof (List<Preset>));

        private readonly string UserPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                                 "\\HandBrake\\user_presets.xml";

        private readonly string HbPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                               "\\HandBrake\\presets.xml";

        /// <summary>
        /// Add a new preset to the system
        /// </summary>
        /// <param name="presetName">String, The name of the new preset</param>
        /// <param name="query">String, the CLI query for the new preset</param>
        /// <param name="pictureSettings"> Bool, store crop/picture sizes in the Presets</param>
        public bool Add(string presetName, string query, bool pictureSettings)
        {
            if (CheckIfPresetExists(presetName) == false)
            {
                Preset newPreset = new Preset
                                       {
                                           Name = presetName, 
                                           Query = query, 
                                           PictureSettings = pictureSettings, 
                                           Version = Properties.Settings.Default.hb_version
                                       };
                UserPresets.Add(newPreset);
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
            foreach (Preset item in Presets)
            {
                if (item.Name != name)
                {
                    newPresets.Add(item);
                }
            }
            Presets = newPresets;

            // User Presets
            foreach (Preset item in UserPresets)
            {
                if (item.Name != name)
                {
                    newUserPresets.Add(item);
                }
            }
            UserPresets = newUserPresets;

            // Rebuild the UserPresets.xml file
            UpdatePresetFiles();
            UpdatePresetFiles();
        }

        /// <summary>
        /// Remove all built in Presets;
        /// </summary>
        public void RemoveBuiltInPresets()
        {
            Presets.Clear();
            UpdatePresetFiles();
        }

        /// <summary>
        /// Save changes to a given preset in the user preset list.
        /// </summary>
        /// <param name="presetName">String, The name of the new preset</param>
        /// <param name="query">String, the CLI query for the new preset</param>
        /// <param name="pictureSettings"> Bool, store crop/picture sizes in the preset</param>
        public void Update(string presetName, string query, bool pictureSettings)
        {
            // User Presets
            foreach (Preset item in UserPresets)
            {
                if (item.Name == presetName)
                {
                    item.Query = query;
                    item.PictureSettings = pictureSettings;
                    MessageBox.Show("Changes to \"" + presetName + "\" Saved", "Success", MessageBoxButtons.OK, 
                                    MessageBoxIcon.Information);
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
            foreach (Preset item in Presets)
            {
                if (item.Name == name)
                    return item;
            }

            // User Presets
            foreach (Preset item in UserPresets)
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
            // Create a new tempory file and execute the CLI to get the built in Presets.
            string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
            string presetsPath = Path.Combine(Path.GetTempPath(), "temp_presets.dat");
            string strCmdLine = String.Format(@"cmd /c """"{0}"" --preset-list >""{1}"" 2>&1""", handbrakeCLIPath, 
                                              presetsPath);

            ProcessStartInfo hbGetPresets = new ProcessStartInfo("CMD.exe", strCmdLine)
                                                {
                                                   WindowStyle = ProcessWindowStyle.Hidden
                                                };
            Process hbproc = Process.Start(hbGetPresets);
            if (hbproc != null)
            {
                hbproc.WaitForExit();
                hbproc.Dispose();
                hbproc.Close();
            }

            // Clear the current built in Presets and now parse the tempory Presets file.
            Presets.Clear();

            if (File.Exists(presetsPath))
            {
                StreamReader presetInput = new StreamReader(presetsPath);

                string category = String.Empty;

                while (!presetInput.EndOfStream)
                {
                    string line = presetInput.ReadLine();
                    if (line.Contains("<") && !line.Contains("<<")) // Found the beginning of a preset block 
                        category = line.Replace("<", string.Empty).Trim();

                    if (line.Contains("+")) // A Preset
                    {
                        Regex r = new Regex("(:  )"); // Split on hyphens. 
                        string[] presetName = r.Split(line);

                        bool pic = false;
                        if (presetName[2].Contains("crop"))
                            pic = true;

                        Preset newPreset = new Preset
                                               {
                                                   Category = category, 
                                                   Name = presetName[0].Replace("+", string.Empty).Trim(), 
                                                   Query = presetName[2], 
                                                   Version = Properties.Settings.Default.hb_version, 
                                                   PictureSettings = pic
                                               };
                        Presets.Add(newPreset);
                    }
                }
                presetInput.Close();
                presetInput.Dispose();
            }

            // Finally, Create a new or update the current Presets.xml file
            UpdatePresetFiles();
        }

        /// <summary>
        /// Load in the preset data from Presets.xml and UserPresets.xml
        /// Load it into the 2 arraylist's Presets and UserPresets
        /// </summary>
        private void LoadPresetData()
        {
            // First clear the Presets arraylists
            Presets.Clear();
            UserPresets.Clear();

            try
            {
                // Load in the users Presets from UserPresets.xml
                if (File.Exists(HbPresetFile))
                {
                    using (FileStream strm = new FileStream(HbPresetFile, FileMode.Open, FileAccess.Read))
                    {
                        if (strm.Length != 0)
                        {
                            List<Preset> list = Ser.Deserialize(strm) as List<Preset>;

                            if (list != null)
                                foreach (Preset preset in list)
                                    Presets.Add(preset);
                        }
                    }
                }
            }
            catch (Exception)
            {
                MessageBox.Show(
                    "HandBrakes preset file appears to have been corrupted. This file will now be re-generated!\n" +
                    "If the problem presists, please delete the file: \n\n" + HbPresetFile, "Error", 
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                UpdateBuiltInPresets();
            }

            try
            {
                // Load in the users Presets from UserPresets.xml
                if (File.Exists(UserPresetFile))
                {
                    using (FileStream strm = new FileStream(UserPresetFile, FileMode.Open, FileAccess.Read))
                    {
                        if (strm.Length != 0)
                        {
                            List<Preset> list = Ser.Deserialize(strm) as List<Preset>;

                            if (list != null)
                                foreach (Preset preset in list)
                                    UserPresets.Add(preset);
                        }
                    }
                }
            }
            catch (Exception)
            {
                MessageBox.Show(
                    "Your User presets file appears to have been corrupted.\n" +
                    "Your presets will not be loaded. You may need to re-create your presets.\n\n" +
                    "Your user presets file has been renamed to 'user_presets.xml.old' and is located in:\n " +
                    Path.GetDirectoryName(UserPresetFile) + "\n" +
                    "You may be able to recover some presets if you know the XML language.", "Error", 
                    MessageBoxButtons.OK, MessageBoxIcon.Error);

                // Recover from Error.
                if (File.Exists(UserPresetFile))
                {
                    string disabledFile = UserPresetFile + ".old";
                    if (File.Exists(disabledFile))
                        File.Delete(disabledFile);
                    File.Move(UserPresetFile, disabledFile);
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
            string category = string.Empty;
            TreeNode rootNode = null;

            if (Presets.Count != 0) // Built In Presets
            {
                foreach (Preset preset in Presets)
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

            rootNode = null;
            category = null;
            foreach (Preset preset in UserPresets) // User Presets
            {
                if (preset.Category != category && preset.Category != null)
                {
                    rootNode = new TreeNode(preset.Category) {ForeColor = Color.Black};
                    presetPanel.Nodes.Add(rootNode);
                    category = preset.Category;
                }

                if (preset.Category == category && rootNode != null)
                    rootNode.Nodes.Add(new TreeNode(preset.Name) {ForeColor = Color.Black});
                else
                    presetPanel.Nodes.Add(new TreeNode(preset.Name) {ForeColor = Color.Black});
            }
        }

        /// <summary>
        /// Update the preset files
        /// </summary>
        private void UpdatePresetFiles()
        {
            try
            {
                using (FileStream strm = new FileStream(HbPresetFile, FileMode.Create, FileAccess.Write))
                {
                    Ser.Serialize(strm, Presets);
                    strm.Close();
                    strm.Dispose();
                }

                using (FileStream strm = new FileStream(UserPresetFile, FileMode.Create, FileAccess.Write))
                {
                    Ser.Serialize(strm, UserPresets);
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(
                    "Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n Error Information: \n\n" +
                    exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }
        }

        /// <summary>
        /// Check if the preset "name" exists in either Presets or UserPresets lists.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        private bool CheckIfPresetExists(string name)
        {
            if (name == string.Empty)
                return true;

            // Built In Presets
            foreach (Preset item in Presets)
            {
                if (item.Name == name)
                    return true;
            }

            // User Presets
            foreach (Preset item in UserPresets)
            {
                if (item.Name == name)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Check if the user preset "name" exists in UserPresets list.
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public bool CheckIfUserPresetExists(string name)
        {
            if (name == string.Empty)
                return false;

            // User Presets
            foreach (Preset item in UserPresets)
            {
                if (item.Name == name)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Check if the built in Presets stored are not out of date.
        /// Update them if they are.
        /// </summary>
        /// <returns></returns>
        public bool CheckIfPresetsAreOutOfDate()
        {
            LoadPresetData();
            // Update built-in Presets if the built-in Presets belong to an older version.
            if (Presets.Count != 0)
                if (Presets[0].Version != Properties.Settings.Default.hb_version)
                {
                    UpdateBuiltInPresets();
                    return true;
                }

            return false;
        }
    }
}