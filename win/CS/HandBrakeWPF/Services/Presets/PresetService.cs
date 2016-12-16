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

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.Json.Presets;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Utilities;

    using HandBrakeWPF.Factories;
    using HandBrakeWPF.Model.Picture;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Model.Models;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Factories;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;

    using GeneralApplicationException = HandBrakeWPF.Exceptions.GeneralApplicationException;

    /// <summary>
    /// The preset service manages HandBrake's presets
    /// </summary>
    public class PresetService : IPresetService
    {
        #region Private Variables

        public const int ForcePresetReset = 3;
        public static string UserPresetCatgoryName = "User Presets";
        private readonly string presetFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly()), "presets.json");
        private readonly ObservableCollection<Preset> presets = new ObservableCollection<Preset>();
        private readonly IErrorService errorService;
        private readonly IUserSettingService userSettingService;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="PresetService"/> class.
        /// </summary>
        /// <param name="errorService">
        /// The error service.
        /// </param>
        /// <param name="userSettingService">
        /// The User setting service.
        /// </param>
        public PresetService(IErrorService errorService, IUserSettingService userSettingService)
        {
            this.errorService = errorService;
            this.userSettingService = userSettingService;
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
            if (!File.Exists(this.presetFile))
            {
                this.UpdateBuiltInPresets();
            }

            // Load the presets from file
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
            if (!this.CheckIfPresetExists(preset.Name))
            {
                this.presets.Add(preset);

                // Update the presets file
                this.SavePresetFiles();
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
            if (!string.IsNullOrEmpty(filename))
            {
                PresetTransportContainer container = HandBrakePresetService.GetPresetFromFile(filename);

                if (container?.PresetList == null || container.PresetList.Count == 0)
                {
                    this.errorService.ShowError(Resources.Main_PresetImportFailed, Resources.Main_PresetImportFailedSolution, string.Empty);
                    return;
                }

                // HBPreset Handling
                if (container.PresetList != null)
                {
                    foreach (var objectPreset in container.PresetList)
                    {
                        HBPreset hbPreset = JsonConvert.DeserializeObject<HBPreset>(objectPreset.ToString());

                        Preset preset = null;
                        try
                        {
                            preset = JsonPresetFactory.ImportPreset(hbPreset);
                            preset.Category = UserPresetCatgoryName;

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

                        if (this.CheckIfPresetExists(preset.Name))
                        {
                            if (!this.CanUpdatePreset(preset.Name))
                            {
                                MessageBox.Show(Resources.Main_PresetErrorBuiltInName, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                                return;
                            }

                            MessageBoxResult result = MessageBox.Show(Resources.Main_PresetOverwriteWarning, Resources.Overwrite, MessageBoxButton.YesNo, MessageBoxImage.Warning);
                            if (result == MessageBoxResult.Yes)
                            {
                                this.Update(preset);
                            }
                        }
                        else
                        {
                            this.Add(preset);
                        }
                    }
                }

                // Category Handling.
                // TODO maybe for a future release.
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
            // TODO Add support for multiple export
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
                    preset.PictureSettingsMode = update.PictureSettingsMode;
                    preset.Category = update.Category;
                    preset.Description = update.Description;
                    preset.AudioTrackBehaviours = update.AudioTrackBehaviours;
                    preset.SubtitleTrackBehaviours = update.SubtitleTrackBehaviours;

                    // Update the presets file
                    this.SavePresetFiles();
                    break;
                }
            }
        }

        /// <summary>
        /// Replace an existing preset with a modified one.
        /// </summary>
        /// <param name="existing">
        /// The existing.
        /// </param>
        /// <param name="replacement">
        /// The replacement.
        /// </param>
        public void Replace(Preset existing, Preset replacement)
        {
            this.Remove(existing);
            this.Add(replacement);
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
            this.SavePresetFiles();
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

            this.SavePresetFiles();
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
            this.SavePresetFiles();
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

            foreach (var category in presetCategories)
            {
                foreach (var hbpreset in category.ChildrenArray)
                {
                    Preset preset = JsonPresetFactory.ImportPreset(hbpreset);
                    preset.IsBuildIn = true; 
                    preset.Category = category.PresetName;

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
            this.SavePresetFiles();
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
        private string RecoverFromCorruptedPresetFile(string file)
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
        /// Archive the presets file without deleting it.
        /// </summary>
        /// <param name="file">The filename to archive</param>
        /// <returns>The archived filename</returns>
        private string ArchivePresetFile(string file)
        {
            try
            {
                // Recover from Error.
                string archiveFile = string.Format("{0}.{1}", file, GeneralUtilities.ProcessId);
                if (File.Exists(file))
                {
                    File.Copy(file, archiveFile);
                }

                return archiveFile;
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

            // Load the presets file.
            try
            {
                // If we don't have a presets file. Create one for first load.
                if (!File.Exists(this.presetFile))
                {
                    // If this is a nightly, and we don't have a presets file, try port the main version if it exists.
                    string releasePresetFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(false), "presets.json");
                    if (VersionHelper.IsNightly() && File.Exists(releasePresetFile))
                    {
                        File.Copy(releasePresetFile, DirectoryUtilities.GetUserStoragePath(true));
                    }
                    else
                    {
                        this.UpdateBuiltInPresets();
                        return; // Update built-in presets stores the presets locally, so just return.
                    }
                }

                // Otherwise, we already have a file, so lets try load it.
                PresetTransportContainer container = null;
                using (StreamReader reader = new StreamReader(this.presetFile))
                {
                    try
                    {
                        container = JsonConvert.DeserializeObject<PresetTransportContainer>(reader.ReadToEnd());
                    }
                    catch (Exception exc)
                    {
                        Debug.WriteLine("Failed to parse presets file: " + exc);
                    }
                }

                // Sanity Check. Did the container deserialise.
                if (container == null || container.PresetList == null)
                {
                    string filename = this.RecoverFromCorruptedPresetFile(this.presetFile);
                    this.errorService.ShowMessageBox(
                       Resources.PresetService_UnableToLoadPresets + filename,
                       Resources.PresetService_UnableToLoad,
                       MessageBoxButton.OK,
                       MessageBoxImage.Exclamation);

                    this.UpdateBuiltInPresets();
                    return; // Update built-in presets stores the presets locally, so just return.
                }

                // Version Check
                // If we have old presets, or the container wasn't parseable, or we have a version mismatch, backup the user preset file 
                // incase something goes wrong and reset built-in presets, then re-save.
                if (container.VersionMajor != Constants.PresetVersionMajor || container.VersionMinor != Constants.PresetVersionMinor || container.VersionMicro != Constants.PresetVersionMicro)
                {
                    string fileName = this.ArchivePresetFile(this.presetFile);
                    this.errorService.ShowMessageBox(
                        Resources.PresetService_PresetsOutOfDate
                        + Environment.NewLine + Environment.NewLine + Resources.PresetService_ArchiveFile + fileName,
                        Resources.PresetService_UnableToLoad,
                        MessageBoxButton.OK,
                        MessageBoxImage.Exclamation);
                    this.UpdateBuiltInPresets(); // Update built-in presets stores the presets locally, so just return.
                    return;
                }

                // Force Upgrade of presets
                if (this.userSettingService.GetUserSetting<int>(UserSettingConstants.ForcePresetReset) < ForcePresetReset)
                {
                    this.userSettingService.SetUserSetting(UserSettingConstants.ForcePresetReset, ForcePresetReset);

                    string fileName = this.ArchivePresetFile(this.presetFile);
                    this.errorService.ShowMessageBox(
                        Resources.Presets_PresetForceReset
                        + Environment.NewLine + Environment.NewLine + Resources.PresetService_ArchiveFile + fileName,
                        Resources.PresetService_UnableToLoad,
                        MessageBoxButton.OK,
                        MessageBoxImage.Exclamation);
                    this.UpdateBuiltInPresets(); // Update built-in presets stores the presets locally, so just return.
                    return;
                }

                // The presets file loaded was OK, so process it.
                foreach (var item in container.PresetList)
                {
                    object deserialisedItem = JsonConvert.DeserializeObject<PresetCategory>(item.ToString());

                    // Handle Categorised Presets.
                    PresetCategory category = deserialisedItem as PresetCategory;
                    if (category != null && category.Folder)
                    {
                        foreach (HBPreset hbpreset in category.ChildrenArray)
                        {
                            Preset preset = JsonPresetFactory.ImportPreset(hbpreset);
                            preset.Category = category.PresetName;
                            preset.IsBuildIn = hbpreset.Type == 0;

                            // IF we are using Source Max, Set the Max Width / Height values.
                            if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
                            {
                                preset.Task.MaxWidth = preset.Task.Height;
                                preset.Task.MaxHeight = preset.Task.Width;
                            }

                            this.presets.Add(preset);
                        }
                    }

                    // Uncategorised Presets
                    deserialisedItem = JsonConvert.DeserializeObject<HBPreset>(item.ToString());
                    HBPreset hbPreset = deserialisedItem as HBPreset;
                    if (hbPreset != null && !hbPreset.Folder)
                    {
                        Preset preset = JsonPresetFactory.ImportPreset(hbPreset);
                        preset.Category = UserPresetCatgoryName;
                        preset.IsBuildIn = hbPreset.Type == 1;

                        // IF we are using Source Max, Set the Max Width / Height values.
                        if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
                        {
                            preset.Task.MaxWidth = preset.Task.Height;
                            preset.Task.MaxHeight = preset.Task.Width;
                        }

                        this.presets.Add(preset);
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex);
                this.RecoverFromCorruptedPresetFile(this.presetFile);
                this.UpdateBuiltInPresets();
            }
        }

        /// <summary>
        /// Update the preset files
        /// </summary>
        private void SavePresetFiles()
        {
            try
            {
                // Verify Directories.
                string directory = Path.GetDirectoryName(this.presetFile);
                if (directory != null && !Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                // Orgamise the Presets list into Json Equivilent objects.
                Dictionary<string, PresetCategory> presetCategories = new Dictionary<string, PresetCategory>();
                List<HBPreset> uncategorisedPresets = new List<HBPreset>();      
                foreach (Preset item in this.presets)
                {
                    if (string.IsNullOrEmpty(item.Category))
                    {
                        uncategorisedPresets.Add(JsonPresetFactory.CreateHbPreset(item, HBConfigurationFactory.Create()));
                    }
                    else
                    {
                        HBPreset preset = JsonPresetFactory.CreateHbPreset(item, HBConfigurationFactory.Create());
                        if (presetCategories.ContainsKey(item.Category))
                        {
                            presetCategories[item.Category].ChildrenArray.Add(preset);
                        }
                        else
                        {
                            presetCategories[item.Category] = new PresetCategory
                                                              {
                                                                  ChildrenArray = new List<HBPreset> { preset },
                                                                  Folder = true,
                                                                  PresetName = item.Category,
                                                                  Type = item.IsBuildIn ? 0 : 1
                                                              };
                        }
                    }
                }

                // Wrap the categories in a container. 
                JsonSerializerSettings settings = new JsonSerializerSettings { MissingMemberHandling = MissingMemberHandling.Ignore };
                PresetTransportContainer container = new PresetTransportContainer(
                    Constants.PresetVersionMajor,
                    Constants.PresetVersionMinor,
                    Constants.PresetVersionMicro) { PresetList = new List<object>() };
                container.PresetList.AddRange(presetCategories.Values);
                container.PresetList.AddRange(uncategorisedPresets);

                // Write the preset container out to file.
                using (FileStream strm = new FileStream(this.presetFile, FileMode.Create, FileAccess.Write))
                {
                    string presetsJson = JsonConvert.SerializeObject(container, Formatting.Indented, settings);
                    using (StreamWriter writer = new StreamWriter(strm))
                    {
                        writer.WriteLine(presetsJson);
                    }
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
                throw new GeneralApplicationException("Unable to write to the presets file.", "The details section below may indicate why this error has occured.", exc);
            }
        }

        #endregion
    }
}
