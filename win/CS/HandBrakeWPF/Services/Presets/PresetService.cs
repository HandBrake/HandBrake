// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preset service manages HandBrake's presets
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;

    using Newtonsoft.Json;

    /// <summary>
    /// The preset service manages HandBrake's presets
    /// </summary>
    public class PresetService : IPresetService
    {

        // TODO refactor filename

        #region Private Variables

        private static readonly int CurrentPresetVersion = 1;

        /// <summary>
        /// XML Serializer
        /// </summary>
        private static readonly XmlSerializer Ser = new XmlSerializer(typeof(List<Preset>));

        /// <summary>
        /// User Preset Default Catgory Name
        /// </summary>
        public static string UserPresetCatgoryName = "User Presets";

        /// <summary>
        /// The User Preset file
        /// </summary>
        private readonly string userPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\user_presets.json";

        /// <summary>
        /// The Legacy Preset file
        /// </summary>
        private readonly string legacyUserPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\user_presets.xml";


        /// <summary>
        /// The Built In Presets File
        /// </summary>
        private readonly string builtInPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\presets.xml";

        /// <summary>
        /// A Collection of presets
        /// </summary>
        private readonly ObservableCollection<Preset> presets = new ObservableCollection<Preset>();

        #endregion

        /// <summary>
        /// Gets a Collection of presets.
        /// </summary>
        public ObservableCollection<Preset> Presets
        {
            get
            {
                return this.presets;
            }
        }

        /// <summary>
        /// Gets or sets LastPresetAdded.
        /// </summary>
        public Preset LastPresetAdded { get; set; }

        /// <summary>
        /// Gets the DefaultPreset.
        /// </summary>
        public Preset DefaultPreset
        {
            get
            {
                return this.presets.FirstOrDefault(p => p.IsDefault);
            }
        }

        #region Public Methods

        /// <summary>
        /// The load.
        /// </summary>
        public void Load()
        {
            // If the preset file doesn't exist. Create it.
            if (!File.Exists(this.builtInPresetFile))
            {
                this.UpdateBuiltInPresets();
            }

            this.LoadPresets();
        }

        /// <summary>
        /// Add a new preset to the system.
        /// Performs an Update if it already exists
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

            this.Update(preset);
            return true;
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
                    preset.Task = update.Task;
                    preset.UsePictureFilters = update.UsePictureFilters;
                    preset.PictureSettingsMode = update.PictureSettingsMode;
                    preset.Category = update.Category;
                    preset.Description = update.Description;

                    // Update the presets file
                    this.UpdatePresetFiles();
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
            if (preset == null || preset.IsDefault)
            {
                return;
            }

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
                if (preset.IsDefault)
                {
                    // Skip default preset
                    continue;
                }

                this.presets.Remove(preset);
            }

            this.UpdatePresetFiles();
        }

        /// <summary>
        /// Set Default Preset
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        public void SetDefault(Preset name)
        {
            foreach (Preset preset in this.presets)
            {
                preset.IsDefault = false;
            }

            name.IsDefault = true;
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
        public void UpdateBuiltInPresets()
        {
            // Clear the current built in Presets and now parse the tempory Presets file.
            this.ClearBuiltIn();

            using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("HandBrakeWPF.Presets.dat"))
            {
                using (StreamReader reader = new StreamReader(stream))
                {
                    StringBuilder contents = new StringBuilder();

                    string category = String.Empty;

                    while (!reader.EndOfStream)
                    {
                        string line = reader.ReadLine();
                        contents.AppendLine(line);

                        // Found the beginning of a preset block )
                        if (line != null && line.Contains("<") && !line.Contains("<<"))
                        {
                            category = line.Replace("<", string.Empty).Trim();
                        }

                        // Found a preset
                        if (line != null && line.Contains("+"))
                        {
                            Regex r = new Regex("(:  )"); // Split on hyphens. 
                            string[] presetName = r.Split(line);

                            Preset newPreset = new Preset
                            {
                                Category = category,
                                Name = presetName[0].Replace("+", string.Empty).Trim(),
                                Version = VersionHelper.GetVersion(),
                                Description = string.Empty, // Maybe one day we will populate this.
                                IsBuildIn = true,
                                UsePictureFilters = true,
                                Task = QueryParserUtility.Parse(presetName[2])
                            };

                            if (newPreset.Name == "iPod")
                            {
                                newPreset.Task.KeepDisplayAspect = true;
                            }

                            newPreset.Task.AllowedPassthruOptions = new AllowedPassthru(true); // We don't want to override the built-in preset

                            if (newPreset.Name == "Normal")
                            {
                                newPreset.IsDefault = true;
                            }

                            this.presets.Add(newPreset);
                        }
                    }

                    // Verify we have presets.
                    if (this.presets.Count == 0)
                    {
                        throw new GeneralApplicationException("Failed to load built-in presets.", "Restarting HandBrake may resolve this issue", new Exception(contents.ToString()));
                    }
                }
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
                List<Preset> preset = this.presets.Where(p => p.IsBuildIn).ToList();
                if (preset.Count > 0)
                {
                    if (preset[0].Version != VersionHelper.GetVersion())
                    {
                        this.UpdateBuiltInPresets();
                        return true;
                    }
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
            try
            {
                // Recover from Error.
                if (File.Exists(file))
                {
                    string disabledFile = string.Format("{0}.{1}", file, GeneralUtilities.ProcessId);
                    File.Move(file, disabledFile);
                    if (File.Exists(file))
                    {
                        File.Delete(file);
                    }
                }
            }
            catch (IOException)
            {
                // Give up
            }
        }

        /// <summary>
        /// Load in the Built-in and User presets into the collection
        /// </summary>
        private void LoadPresets()
        {
            // First clear the Presets arraylists
            this.presets.Clear();

            // Load in the Presets from Presets.xml
            try
            {
                if (File.Exists(this.builtInPresetFile))
                {
                    using (StreamReader reader = new StreamReader(this.builtInPresetFile))
                    {
                        List<Preset> list = (List<Preset>)Ser.Deserialize(reader);
                        foreach (Preset preset in list)
                        {
                            preset.IsBuildIn = true;
                            // Older versions did not have this flag so explicitly make sure it is set.
                            this.presets.Add(preset);
                        }
                    }
                }
            }
            catch (Exception)
            {
                RecoverFromCorruptedPresetFile(this.builtInPresetFile);
                this.UpdateBuiltInPresets();
            }

            // Load in the users Presets from UserPresets.xml
            try
            { 
                // Handle Legacy Preset Format.
                bool updatePresets = false;
                if (File.Exists(this.legacyUserPresetFile))
                {
                    using (StreamReader reader = new StreamReader(this.legacyUserPresetFile))
                    {
                        try
                        {
                            var oldPresets = (List<Preset>)Ser.Deserialize(reader);
                            foreach (Preset oldPreset in oldPresets)
                            {
                                this.presets.Add(oldPreset);
                            }
                            updatePresets = true;
                            
                        }
                        catch (Exception exc)
                        {
                            // Do Nothing
                        }
                    }

                    // Archive the old file incase the user needs it.
                    File.Move(this.legacyUserPresetFile, this.legacyUserPresetFile + ".archive." + GeneralUtilities.ProcessId);
                }

                // New JSON Format.
                if (File.Exists(this.userPresetFile))
                {
                    // New Preset Format.
                    using (StreamReader reader = new StreamReader(this.userPresetFile))
                    {
                        PresetContainer presetContainer = null;

                        bool createBackup = false;
                        try
                        {
                           presetContainer = JsonConvert.DeserializeObject<PresetContainer>(reader.ReadToEnd());
                        } 
                        catch (Exception exc)
                        {
                            createBackup = true;
                        }

                        // If we have old presets, or the container wasn't parseable, or we have a version mismatch, backup the user preset file 
                        // incase something goes wrong.
                        if (createBackup || (presetContainer != null && presetContainer.Version < CurrentPresetVersion))
                        {
                            string backupFile = this.userPresetFile + "." + GeneralUtilities.ProcessId;
                            File.Copy(this.userPresetFile, backupFile);
                        }

                        // Load the current presets.
                        if (presetContainer != null && !string.IsNullOrEmpty(presetContainer.Presets))
                        {
                            JsonSerializerSettings settings = new JsonSerializerSettings();
                            settings.MissingMemberHandling = MissingMemberHandling.Ignore;
                            List<Preset> list = JsonConvert.DeserializeObject<List<Preset>>(presetContainer.Presets);
                            foreach (Preset preset in list)
                            {
                                this.presets.Add(preset);
                            }
                        }
                    }
                }

                // We did a preset convertion, so save the updates.
                if (updatePresets)
                {
                    UpdatePresetFiles();
                }
            }
            catch (Exception exc)
            {
                RecoverFromCorruptedPresetFile(this.userPresetFile);
                throw new GeneralApplicationException("HandBrake has detected a problem with your presets.", "Your old presets file has been renamed so that it doesn't get loaded on next launch.", exc);
            }
        }

        /// <summary>
        /// Update the preset files
        /// </summary>
        private void UpdatePresetFiles()
        {
            try
            {
                string directory = Path.GetDirectoryName(this.userPresetFile);
                if (!Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                using (FileStream strm = new FileStream(this.builtInPresetFile, FileMode.Create, FileAccess.Write))
                {
                    Ser.Serialize(strm, this.presets.Where(p => p.IsBuildIn).ToList());
                }

                using (FileStream strm = new FileStream(this.userPresetFile, FileMode.Create, FileAccess.Write))
                {
                    List<Preset> userPresets = this.presets.Where(p => p.IsBuildIn == false).ToList();
                    string presetsJson = JsonConvert.SerializeObject(userPresets, Formatting.Indented);

                    PresetContainer container = new PresetContainer(CurrentPresetVersion, presetsJson);
                    string containerJson = JsonConvert.SerializeObject(container, Formatting.Indented);

                    using (StreamWriter writer = new StreamWriter(strm))
                    {
                        writer.WriteLine(containerJson);
                    }
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException("Unable to write to the presets file.", "The details section below may indicate why this error has occured.", exc);
            }
        }

        #endregion
    }
}
