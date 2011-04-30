/*  PresetService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The preset service manages HandBrake's presets
    /// </summary>
    public class PresetService : IPresetService
    {
        /**
         * TODO:
         * - Wire this into the Forms and WPF UI's
         * - Note: This is untested so far. It'll likely need fixes before it can be used.
         * - Maybe change the collection to a dictionary to allow easier lookups?
         **/

        #region Private Variables

        /// <summary>
        /// XML Serializer
        /// </summary>
        private static readonly XmlSerializer Ser = new XmlSerializer(typeof(List<Preset>));

        /// <summary>
        /// The User Preset file
        /// </summary>
        private readonly string userPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\user_presets.xml";

        /// <summary>
        /// The Built In Presets File
        /// </summary>
        private readonly string builtInPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\presets.xml";

        /// <summary>
        /// A Collection of presets
        /// </summary>
        private ObservableCollection<Preset> presets = new ObservableCollection<Preset>();

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="PresetService"/> class.
        /// </summary>
        public PresetService()
        {
            // this.Presets = CollectionViewSource.GetDefaultView(this.presets);
            this.LoadPresets();
        }

        /// <summary>
        /// Gets or sets a Collection of presets.
        /// </summary>
        public ObservableCollection<Preset> Presets
        {
            get
            {
                return this.presets;
            }

            set
            {
                this.presets = value;
            }
        }

        /// <summary>
        /// The last preset added.
        /// </summary>
        public Preset LastPresetAdded { get; set; }

        #region Public Methods

        /// <summary>
        /// Add a new preset to the system
        /// </summary>
        /// <param name="preset">
        /// A Preset to add
        /// </param>
        /// <returns>
        /// True if added,
        /// False if name already exists
        /// </returns>
        public bool Add(Preset preset)
        {
            if (this.CheckIfPresetExists(preset.Name) == false)
            {
                this.presets.Add(preset);
                this.LastPresetAdded = preset;

                // Update the presets file
                this.UpdatePresetFiles();
                return true;
            }

            return false;
        }

        /// <summary>
        /// Update a preset
        /// </summary>
        /// <param name="update">
        /// The updated preset
        /// </param>
        public void Update(Preset update)
        {
            // TODO - Change this to be a lookup
            foreach (Preset preset in this.presets)
            {
                if (preset.Name == update.Name)
                {
                    preset.Query = update.Query;
                    break;
                }
            }
        }

        /// <summary>
        /// Remove a preset with a given name from either the built in or user preset list.
        /// </summary>
        /// <param name="preset">
        /// The Preset to remove
        /// </param>
        public void Remove(Preset preset)
        {
            this.presets.Remove(preset);
            this.UpdatePresetFiles();
        }

        /// <summary>
        /// Remove a group of presets by category
        /// </summary>
        /// <param name="category">
        /// The Category to remove
        /// </param>
        public void RemoveGroup(string category)
        {
            List<Preset> removeList = this.presets.Where(p => p.Category == category).ToList();
            foreach (Preset preset in removeList)
            {
                this.presets.Remove(preset);
            }

            this.UpdatePresetFiles();
        }

        /// <summary>
        /// Get a Preset
        /// </summary>
        /// <param name="name">
        /// The name of the preset to get
        /// </param>
        /// <returns>
        /// A Preset or null object
        /// </returns>
        public Preset GetPreset(string name)
        {
            return this.presets.FirstOrDefault(item => item.Name == name);
        }

        /// <summary>
        /// Clear Built-in Presets
        /// </summary>
        public void ClearBuiltIn()
        {
            List<Preset> remove = this.presets.Where(p => p.IsBuildIn).ToList();
            foreach (Preset preset in remove)
            {
                this.presets.Remove(preset);
            }
        }

        /// <summary>
        /// Clear all presets
        /// </summary>
        public void ClearAll()
        {
            this.presets.Clear();
        }

        /// <summary>
        /// Reads the CLI's CLI output format and load's them into the preset List Preset
        /// </summary>
        /// <param name="cliPath">
        /// The Path to the CLI, leave blank for current folder.
        /// </param>
        public void UpdateBuiltInPresets(string cliPath)
        {
            // Create a new tempory file and execute the CLI to get the built in Presets.
            string handbrakeCLIPath = Path.Combine(cliPath, "HandBrakeCLI.exe");
            string presetsPath = Path.Combine(Path.GetTempPath(), "temp_presets.dat");
            string strCmdLine = String.Format(@"cmd /c """"{0}"" --preset-list >""{1}"" 2>&1""", handbrakeCLIPath, presetsPath);

            ProcessStartInfo getPresets = new ProcessStartInfo("CMD.exe", strCmdLine) { WindowStyle = ProcessWindowStyle.Hidden };

            Process hbproc = Process.Start(getPresets);
            hbproc.WaitForExit();
            hbproc.Dispose();
            hbproc.Close();

            // Clear the current built in Presets and now parse the tempory Presets file.
            this.ClearBuiltIn();

            if (File.Exists(presetsPath))
            {
                StreamReader presetInput = new StreamReader(presetsPath);

                string category = String.Empty;

                while (!presetInput.EndOfStream)
                {
                    string line = presetInput.ReadLine();

                    // Found the beginning of a preset block 
                    if (line != null && line.Contains("<") && !line.Contains("<<"))
                    {
                        category = line.Replace("<", string.Empty).Trim();
                    }

                    // Found a preset
                    if (line != null && line.Contains("+"))
                    {
                        Regex r = new Regex("(:  )"); // Split on hyphens. 
                        string[] presetName = r.Split(line);

                        bool pic = false;
                        if (presetName[2].Contains("crop"))
                        {
                            pic = true;
                        }

                        Preset newPreset = new Preset
                            {
                                Category = category,
                                Name = presetName[0].Replace("+", string.Empty).Trim(),
                                Query = presetName[2],
                                Version = Properties.Settings.Default.HandBrakeVersion,
                                CropSettings = pic,
                                Description = string.Empty, // Maybe one day we will populate this.
                                IsBuildIn = true
                            };
                        this.presets.Add(newPreset);
                    }
                }
                presetInput.Close();
                presetInput.Dispose();
            }

            // Finally, Create a new or update the current Presets.xml file
            this.UpdatePresetFiles();
        }

        /// <summary>
        /// Check if the built in Presets stored are not out of date.
        /// Update them if they are.
        /// </summary>
        /// <returns>true if out of date</returns>
        public bool CheckIfPresetsAreOutOfDate()
        {
            // Update built-in Presets if the built-in Presets belong to an older version.
            if (this.presets.Count != 0)
            {
                if (this.presets[0].Version != Properties.Settings.Default.HandBrakeVersion)
                {
                    this.UpdateBuiltInPresets(string.Empty);
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Check if the preset "name" exists in either Presets or UserPresets lists.
        /// </summary>
        /// <param name="name">Name of the preset</param>
        /// <returns>True if found</returns>
        public bool CheckIfPresetExists(string name)
        {
            return name == string.Empty || this.presets.Any(item => item.Name == name);
        }

        /// <summary>
        /// Returns a value if the preset can be updated / resaved
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// True if it's not a built-in preset, false otherwise.
        /// </returns>
        public bool CanUpdatePreset(string name)
        {
            return this.presets.Where(preset => preset.Name == name).Any(preset => preset.IsBuildIn == false);
        }

        #endregion

        #region Private Helpers

        /// <summary>
        /// Recover from a courrpted preset file
        /// Add .old to the current filename, and delete the current file.
        /// </summary>
        /// <param name="file">
        /// The broken presets file.
        /// </param>
        private static void RecoverFromCorruptedPresetFile(string file)
        {
            // Recover from Error.
            if (File.Exists(file))
            {
                string disabledFile = file + ".old";
                File.Move(file, disabledFile);
                if (File.Exists(file))
                {
                    File.Delete(file);
                }
            }
        }

        /// <summary>
        /// Load in the Built-in and User presets into the collection
        /// </summary>
        private void LoadPresets()
        {
            // First clear the Presets arraylists
            this.presets.Clear();

            // Load in the users Presets from UserPresets.xml
            try
            {
                if (File.Exists(this.builtInPresetFile))
                {
                    StreamReader reader = new StreamReader(this.builtInPresetFile);
                    List<Preset> list = (List<Preset>)Ser.Deserialize(reader);
                    foreach (Preset preset in list)
                    {
                        this.presets.Add(preset);
                    }

                    reader.Close();
                }
            }
            catch (Exception)
            {
                RecoverFromCorruptedPresetFile(this.builtInPresetFile);
                this.UpdateBuiltInPresets(string.Empty);
            }

            // Load in the users Presets from UserPresets.xml
            try
            {
                if (File.Exists(this.userPresetFile))
                {
                    StreamReader reader = new StreamReader(this.userPresetFile);
                    List<Preset> list = (List<Preset>)Ser.Deserialize(reader);
                    foreach (Preset preset in list)
                    {
                        this.presets.Add(preset);
                    }

                    reader.Close();
                }
            }
            catch (Exception)
            {
                RecoverFromCorruptedPresetFile(this.userPresetFile);
            }
        }

        /// <summary>
        /// Update the preset files
        /// </summary>
        private void UpdatePresetFiles()
        {
            try
            {
                using (FileStream strm = new FileStream(this.builtInPresetFile, FileMode.Create, FileAccess.Write))
                {
                    Ser.Serialize(strm, this.presets.Where(p => p.IsBuildIn).ToList());
                    strm.Close();
                    strm.Dispose();
                }

                using (FileStream strm = new FileStream(this.userPresetFile, FileMode.Create, FileAccess.Write))
                {
                    Ser.Serialize(strm, this.presets.Where(p => p.IsBuildIn == false).ToList());
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception exc)
            {
                throw new Exception("Unable to write to the file. Please make sure the location has the correct permissions for file writing.\n Error Information: \n\n", exc);
            }
        }

        #endregion
    }
}
