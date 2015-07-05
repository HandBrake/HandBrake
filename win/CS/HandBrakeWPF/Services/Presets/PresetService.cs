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
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Windows;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.Json.Presets;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model.Models;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Model.Subtitles;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Factories;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;

    using Newtonsoft.Json;

    /// <summary>
    /// The preset service manages HandBrake's presets
    /// </summary>
    public class PresetService : IPresetService
    {
        #region Private Variables

        private static readonly int CurrentPresetVersion = 4;

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
        private readonly string builtInPresetFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\presets.json";

        /// <summary>
        /// A Collection of presets
        /// </summary>
        private readonly ObservableCollection<Preset> presets = new ObservableCollection<Preset>();

        /// <summary>
        ///  The Error Service.
        /// </summary>
        private readonly IErrorService errorService;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="PresetService"/> class.
        /// </summary>
        /// <param name="errorService">
        /// The error service.
        /// </param>
        public PresetService(IErrorService errorService)
        {
            this.errorService = errorService;
        }

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

            // Load the presets from file
            this.LoadPresets();

            // Check they are up-to-date.
            if (this.CheckIfPresetsAreOutOfDate())
            {
                this.UpdateBuiltInPresets();
                this.LoadPresets(); // Reload again.
            }
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
        /// The import.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        public void Import(string filename)
        {
            // TODO needs a tidy up but will do for now.
            if (!string.IsNullOrEmpty(filename))
            {
                PresetTransportContainer container = HandBrakePresetService.GetPresetFromFile(filename);

                if (container == null || container.PresetList == null || container.PresetList.Count == 0)
                {
                    this.errorService.ShowError(Resources.Main_PresetImportFailed, Resources.Main_PresetImportFailedSolution, string.Empty);
                    return;
                }

                HBPreset hbPreset = container.PresetList.FirstOrDefault();

                Preset preset = null;
                try
                {
                    preset = JsonPresetFactory.ImportPreset(hbPreset);
                    preset.Category = UserPresetCatgoryName;

                    // Depending on the selected preset options, we may need to change some settings around.
                    // If the user chose not to use fitlers, remove them.
                    if (!preset.UsePictureFilters)
                    {
                        preset.Task.Detelecine = Detelecine.Off;
                        preset.Task.Denoise = Denoise.Off;
                        preset.Task.Deinterlace = Deinterlace.Off;
                        preset.Task.Decomb = Decomb.Off;
                        preset.Task.Deblock = 0;
                        preset.Task.Grayscale = false;
                    }

                    // IF we are using Source Max, Set the Max Width / Height values.
                    if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
                    {
                        preset.Task.MaxWidth = preset.Task.Height;
                        preset.Task.MaxHeight = preset.Task.Width;
                    }
                }
                catch (Exception exc)
                {
                    this.errorService.ShowError(Resources.Main_PresetImportFailed, Resources.Main_PresetImportFailedSolution, exc);
                }

                if (preset == null)
                {
                    this.errorService.ShowError(Resources.Main_PresetImportFailed, Resources.Main_PresetImportFailedSolution, string.Empty);
                    return;
                }

                // TODO Better version checking.
                
                if (this.CheckIfPresetExists(preset.Name))
                {
                    if (!CanUpdatePreset(preset.Name))
                    {
                        MessageBox.Show(Resources.Main_PresetErrorBuiltInName, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                        return;
                    }

                    MessageBoxResult result = MessageBox.Show(Resources.Main_PresetOverwriteWarning, Resources.Overwrite, MessageBoxButton.YesNo, MessageBoxImage.Warning);
                    if (result == MessageBoxResult.Yes)
                    {
                        Update(preset);
                    }
                }
                else
                {
                    Add(preset);
                }
            }
        }

        /// <summary>
        /// The export.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        public void Export(string filename, Preset preset, HBConfiguration configuration)
        {
            PresetTransportContainer container = JsonPresetFactory.ExportPreset(preset, configuration);
            HandBrakePresetService.ExportPreset(filename, container);
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
                    preset.AudioTrackBehaviours = update.AudioTrackBehaviours;
                    preset.SubtitleTrackBehaviours = update.SubtitleTrackBehaviours;

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

            IList<PresetCategory> presetCategories = HandBrakePresetService.GetBuiltInPresets();

            foreach (var item in presetCategories)
            {
                foreach (var hbpreset in item.ChildrenArray)
                {
                    Preset preset = JsonPresetFactory.ImportPreset(hbpreset);
                    preset.Version = VersionHelper.GetVersion();
                    preset.UsePictureFilters = true;
                    preset.IsBuildIn = true; // Older versions did not have this flag so explicitly make sure it is set.
                    preset.Category = item.PresetName;

                    if (preset.Name == "iPod")
                    {
                        preset.Task.KeepDisplayAspect = true;
                    }

                    preset.Task.AllowedPassthruOptions = new AllowedPassthru(true); // We don't want to override the built-in preset

                    if (preset.Name == "Normal")
                    {
                        preset.IsDefault = true;
                    }

                    this.presets.Add(preset);
                }
            }

            // Verify we have presets.
            if (this.presets.Count == 0)
            {
                throw new GeneralApplicationException("Failed to load built-in presets.", "Restarting HandBrake may resolve this issue", null);
            }

            // Store the changes to disk
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
        /// <param name="name">
        /// Name of the preset
        /// </param>
        /// <returns>
        /// True if found
        /// </returns>
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
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        private static string RecoverFromCorruptedPresetFile(string file)
        {
            try
            {
                // Recover from Error.
                string disabledFile = string.Format("{0}.{1}", file, GeneralUtilities.ProcessId);
                if (File.Exists(file))
                {
                    File.Move(file, disabledFile);
                    if (File.Exists(file))
                    {
                        File.Delete(file);
                    }
                }

                return disabledFile;
            }
            catch (IOException)
            {
                // Give up
            }

            return "Sorry, the archiving failed.";
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
                        // New Preset Format.
                        try
                        {
                            var presetList = JsonConvert.DeserializeObject<List<Preset>>(reader.ReadToEnd());

                            foreach (Preset preset in presetList)
                            {
                                preset.IsBuildIn = true;  // Older versions did not have this flag so explicitly make sure it is set.
                                this.presets.Add(preset);
                            }
                        }
                        catch (Exception exc)
                        {
                            // Do Nothing.
                            Debug.WriteLine(exc);
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
                            XmlSerializer Ser = new XmlSerializer(typeof(List<Preset>));
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
                            Debug.WriteLine(exc);
                        }
                    }

                    // Archive the old file incase the user needs it.
                    File.Move(this.legacyUserPresetFile, this.legacyUserPresetFile + ".archive." + GeneralUtilities.ProcessId);
                }

                // New JSON Format.
                if (File.Exists(this.userPresetFile))
                {
                    // New Preset Format.
                    bool createBackup = false;
                    PresetContainer presetContainer = null;
                    using (StreamReader reader = new StreamReader(this.userPresetFile))
                    {
                        try
                        {
                            presetContainer = JsonConvert.DeserializeObject<PresetContainer>(reader.ReadToEnd());
                        }
                        catch (Exception exc)
                        {
                            createBackup = true;
                            Debug.WriteLine(exc);
                        }
                    }

                    // If we have old presets, or the container wasn't parseable, or we have a version mismatch, backup the user preset file 
                    // incase something goes wrong.
                    if (createBackup || (presetContainer != null && presetContainer.Version < CurrentPresetVersion))
                    {
                        string fileName = RecoverFromCorruptedPresetFile(this.userPresetFile);
                        this.errorService.ShowMessageBox(
                            "HandBrake is unable to load your user presets because they are from an older version of HandBrake. Your old presets file has been renamed so that it doesn't get loaded on next launch."
                            + Environment.NewLine + Environment.NewLine + "Archived File: " + fileName, 
                            "Unable to load user presets.", 
                            MessageBoxButton.OK, 
                            MessageBoxImage.Exclamation);
                        return;
                    }

                    // Load the current presets.
                    if (presetContainer != null && !string.IsNullOrEmpty(presetContainer.Presets))
                    {
                        JsonSerializerSettings settings = new JsonSerializerSettings { MissingMemberHandling = MissingMemberHandling.Ignore };
                        List<Preset> list = JsonConvert.DeserializeObject<List<Preset>>(presetContainer.Presets, settings);
                        foreach (Preset preset in list)
                        {
                            this.presets.Add(preset);
                        }
                    }
                }

                // We did a preset convertion, so save the updates.
                if (updatePresets)
                {
                    this.UpdatePresetFiles();
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
                // Setup
                string directory = Path.GetDirectoryName(this.userPresetFile);
                if (!Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                JsonSerializerSettings settings = new JsonSerializerSettings { MissingMemberHandling = MissingMemberHandling.Ignore };

                // Built-in Presets

                using (FileStream strm = new FileStream(this.builtInPresetFile, FileMode.Create, FileAccess.Write))
                {
                    string presetsJson = JsonConvert.SerializeObject(this.presets.Where(p => p.IsBuildIn).ToList(), Formatting.Indented, settings);
                    using (StreamWriter writer = new StreamWriter(strm))
                    {
                        writer.WriteLine(presetsJson);
                    }
                }

                // User Presets
                using (FileStream strm = new FileStream(this.userPresetFile, FileMode.Create, FileAccess.Write))
                {
                    List<Preset> userPresets = this.presets.Where(p => p.IsBuildIn == false).ToList();
                    string presetsJson = JsonConvert.SerializeObject(userPresets, Formatting.Indented, settings);

                    PresetContainer container = new PresetContainer(CurrentPresetVersion, presetsJson);
                    string containerJson = JsonConvert.SerializeObject(container, Formatting.Indented, settings);

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
