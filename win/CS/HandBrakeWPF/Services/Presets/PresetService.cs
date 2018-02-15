﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.Collections.Specialized;
    using System.ComponentModel;
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
        // TODO Strip out the error handling from this service and let upstream UI layer handle it.

        #region Private Variables

        public const int ForcePresetReset = 3;
        public static string UserPresetCatgoryName = "Custom Presets";
        private readonly string presetFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly()), "presets.json");
        private readonly ObservableCollection<IPresetObject> presets = new ObservableCollection<IPresetObject>(); // Can store Presets and PresetDisplayCategory objects.
        private readonly Dictionary<string, Preset> flatPresetList = new Dictionary<string, Preset>();
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
        public ObservableCollection<IPresetObject> Presets
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
                return this.flatPresetList.Values.FirstOrDefault(p => p.IsDefault);
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
                return;
            }

            // Load the presets from file
            this.LoadPresets();
        }

        public bool Add(Preset preset)
        {
            return this.Add(preset, false);
        }

        /// <summary>
        /// Add a new preset to the system.
        /// Performs an Update if it already exists
        /// </summary>
        /// <param name="preset">
        /// A Preset to add
        /// </param>
        /// <param name="isLoading">
        /// Prevents Saving of presets.
        /// </param>
        /// <returns>
        /// True if added,
        /// False if name already exists
        /// </returns>
        public bool Add(Preset preset, bool isLoading)
        {
            if (!this.CheckIfPresetExists(preset.Name))
            {
                // Check to see if the category already exists.
                PresetDisplayCategory category = this.presets.FirstOrDefault(a => a.Category == preset.Category) as PresetDisplayCategory; // TODO build Dict for this.
                if (category != null)
                {
                    category.Presets.Add(preset);
                }
                else if (!string.IsNullOrEmpty(preset.Category))
                {
                    // Otherwise, if we have category but it doesn't exist, create it.
                    this.presets.Add(new PresetDisplayCategory(preset.Category, preset.IsBuildIn, new BindingList<Preset> { preset }));
                }
                else
                {
                    // Preset has no category. 
                    this.presets.Add(preset);
                }

                this.flatPresetList.Add(preset.Name, preset);

                // Update the presets file
                if (!isLoading)
                {
                    this.SavePresetFiles();
                }
                return true;
            }

            return false;
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
                PresetTransportContainer container = null;
                try
                {
                    container = HandBrakePresetService.GetPresetFromFile(filename);
                }
                catch (Exception exc)
                {
                    this.errorService.ShowError(Resources.Main_PresetImportFailed, Resources.Main_PresetImportFailedSolution, exc);
                    return;
                }

                if (container?.PresetList == null || container.PresetList.Count == 0)
                {
                    this.errorService.ShowError(Resources.Main_PresetImportFailed, Resources.Main_PresetImportFailedSolution, Resources.NoAdditionalInformation);
                    return;
                }

                // HBPreset Handling
                if (container.PresetList != null)
                {
                    foreach (var objectPreset in container.PresetList)
                    {
                        PresetCategory category = JsonConvert.DeserializeObject<PresetCategory>(objectPreset.ToString());
                        if (category != null && category.ChildrenArray != null && category.ChildrenArray.Count > 0)
                        {
                            foreach (HBPreset hbPreset in category.ChildrenArray)
                            {
                                Preset preset = this.ConvertHbPreset(hbPreset);
                                if (preset != null)
                                {
                                    this.AddOrUpdateImportedPreset(preset);
                                }
                            }
                        }
                        else
                        {
                            HBPreset hbPreset = JsonConvert.DeserializeObject<HBPreset>(objectPreset.ToString());
                            if (hbPreset != null)
                            {
                                Preset preset = this.ConvertHbPreset(hbPreset);
                                if (preset != null)
                                {
                                    this.AddOrUpdateImportedPreset(preset);
                                }
                            }
                        }
                    }
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
            Preset preset;
            if (this.flatPresetList.TryGetValue(update.Name, out preset))
            {
                preset.Task = update.Task;
                preset.PictureSettingsMode = update.PictureSettingsMode;
                preset.Category = update.Category;
                preset.Description = update.Description;
                preset.AudioTrackBehaviours = update.AudioTrackBehaviours;
                preset.SubtitleTrackBehaviours = update.SubtitleTrackBehaviours;

                // Update the presets file
                this.SavePresetFiles();
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
            this.Add(replacement, false);
        }

        /// <summary>
        /// Remove a preset with a given name from either the built in or user preset list.
        /// </summary>
        /// <param name="preset">
        /// The Preset to remove
        /// </param>
        /// <returns>True if successfully removed, false otherwise.</returns>
        public bool Remove(Preset preset)
        {
            if (preset == null || preset.IsDefault)
            {
                return false;
            }

            PresetDisplayCategory cateogry = this.presets.FirstOrDefault(p => p.Category == preset.Category) as PresetDisplayCategory;
            if (cateogry != null)
            {
                // Remove the preset, and cleanup the category if it's not got any presets in it.
                cateogry.Presets.Remove(preset);
                this.flatPresetList.Remove(preset.Name);
                if (cateogry.Presets.Count == 0)
                {
                    this.presets.Remove(cateogry);
                }
            }
            else
            {
                this.presets.Remove(preset);
                this.flatPresetList.Remove(preset.Name);
            }

            this.SavePresetFiles();

            return true;
        }

        /// <summary>
        /// Remove a group of presets by category
        /// </summary>
        /// <param name="categoryName">
        /// The Category to remove
        /// </param>
        public void RemoveGroup(string categoryName)
        {
            PresetDisplayCategory category = this.presets.FirstOrDefault(p => p.Category == categoryName) as PresetDisplayCategory;
            if (category != null)
            {
                foreach (Preset preset in category.Presets)
                {
                    if (preset.IsDefault)
                    {
                        // Skip default preset
                        continue;
                    }

                    this.presets.Remove(preset);
                    this.flatPresetList.Remove(preset.Name);
                }

                // Cleanup the category if we can.
                if (category.Presets.Count == 0)
                {
                    this.presets.Remove(category);
                }

                this.SavePresetFiles();
            }
        }

        /// <summary>
        /// Set Default Preset
        /// </summary>
        /// <param name="preset">
        /// The name.
        /// </param>
        public void SetDefault(Preset preset)
        {
            // Set IsDefault false for everything.
            foreach (Preset item in this.flatPresetList.Values)
            {
                item.IsDefault = false;
            }

            // Set the new preset to default.
            preset.IsDefault = true;

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
            Preset preset;
            if (this.flatPresetList.TryGetValue(name, out preset))
            {
                return preset;
            }

            return null;
        }

        /// <summary>
        /// Clear Built-in Presets
        /// </summary>
        public void ClearBuiltIn()
        {
            List<IPresetObject> topLevel = new List<IPresetObject>();
            foreach (IPresetObject item in this.presets)
            {
                // We either have a Preset
                Preset foundPreset = item as Preset;
                if (foundPreset != null && foundPreset.IsBuildIn)
                {
                    topLevel.Add(item);
                }

                // Or a Category.
                PresetDisplayCategory foundCategory = item as PresetDisplayCategory;
                if (foundCategory != null)
                {
                    // Find all the presets in this category to remove
                    List<Preset> presetsToRemove = new List<Preset>();
                    foreach (Preset categoryPreset in foundCategory.Presets)
                    {
                        if (categoryPreset.IsBuildIn)
                        {
                            presetsToRemove.Add(categoryPreset);
                        }
                    }

                    // Then remove them.
                    foreach (Preset toRemove in presetsToRemove)
                    {
                        foundCategory.Presets.Remove(toRemove);
                        this.flatPresetList.Remove(toRemove.Name);
                    }

                    // Check if we can remove this category.
                    if (foundCategory.Presets.Count == 0)
                    {
                        topLevel.Add(foundCategory);
                    }
                }
            }

            // Remove any top level items we need to remove.
            foreach (var item in topLevel)
            {
                this.presets.Remove(item);

                if (item.GetType() == typeof(Preset))
                {
                    this.flatPresetList.Remove(((Preset)item).Name);
                }
            }
        }

        /// <summary>
        /// Clear all presets
        /// </summary>
        public void ClearAll()
        {
            this.presets.Clear();
            this.flatPresetList.Clear();
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
                    preset.Task.AllowedPassthruOptions = new AllowedPassthru(true); // We don't want to override the built-in preset

                    this.Add(preset, true);
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
            if (this.flatPresetList.ContainsKey(name))
            {
                return true;
            }

            return false;
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
            Preset preset;
            if (this.flatPresetList.TryGetValue(name, out preset))
            {
                return !preset.IsBuildIn;
            }

            return true;
        }

        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="selectedPreset">The preset we want to select.</param>
        public void SetSelected(Preset selectedPreset)
        {
            foreach (var item in this.flatPresetList.Values)
            {
                item.IsSelected = false;
            }

            selectedPreset.IsSelected = true;
        }

        public void SaveCategoryStates()
        {
            StringCollection expandedPresets = new StringCollection();
            foreach (IPresetObject presetObject in this.presets)
            {
                PresetDisplayCategory category = presetObject as PresetDisplayCategory;
                if (category != null && category.IsExpanded)
                {
                    expandedPresets.Add(category.Category);
                }
            }

            this.userSettingService.SetUserSetting(UserSettingConstants.PresetExpandedStateList, expandedPresets);
        }

        public void LoadCategoryStates()
        {
            StringCollection expandedPresets = this.userSettingService.GetUserSetting<StringCollection>(UserSettingConstants.PresetExpandedStateList);
            if (expandedPresets == null || expandedPresets.Count == 0)
            {
                return;
            }

            foreach (IPresetObject presetObject in this.presets)
            {
                PresetDisplayCategory category = presetObject as PresetDisplayCategory;
                if (category != null && expandedPresets.Contains(category.Category))
                {
                    category.IsExpanded = true;
                }
            }
        }

        public IList<PresetDisplayCategory> GetPresetCategories(bool userCategoriesOnly)
        {
            List<PresetDisplayCategory> categoriesList = new List<PresetDisplayCategory>();

            foreach (var item in this.Presets)
            {
                PresetDisplayCategory category = item as PresetDisplayCategory;
                if (category != null)
                {
                    if (userCategoriesOnly && category.IsBuiltIn)
                    {
                        continue;
                    }
                   
                        categoriesList.Add(category);
                   
                }
            }

            return categoriesList;
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
                            
                            // Migration
                            if (category.PresetName == "User Presets")
                            {
                                preset.Category = UserPresetCatgoryName;
                            }
                            else
                            {
                                preset.Category = category.PresetName;
                            }
                            preset.IsBuildIn = hbpreset.Type == 0;

                            // IF we are using Source Max, Set the Max Width / Height values.
                            if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
                            {
                                preset.Task.MaxWidth = preset.Task.Height;
                                preset.Task.MaxHeight = preset.Task.Width;
                            }

                            this.Add(preset, true);
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

                        this.Add(preset, true);
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

                // Organise the Presets list into Json Equivalent objects.
                Dictionary<string, PresetCategory> presetCategories = new Dictionary<string, PresetCategory>();
                List<HBPreset> uncategorisedPresets = new List<HBPreset>();

                // Handle User Presets first.
                foreach (Preset item in this.flatPresetList.Values.OrderBy(o => o.IsBuildIn))
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
                                                             Constants.PresetVersionMicro)
                                                         { PresetList = new List<object>() };
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
                throw new GeneralApplicationException("Unable to write to the presets file.", "The details section below may indicate why this error has occurred.", exc);
            }
        }

        private Preset ConvertHbPreset(HBPreset hbPreset)
        {
            Preset preset = null;

            preset = JsonPresetFactory.ImportPreset(hbPreset);
            preset.Category = UserPresetCatgoryName; // TODO can we get this from the preset?

            // IF we are using Source Max, Set the Max Width / Height values.
            if (preset.PictureSettingsMode == PresetPictureSettingsMode.SourceMaximum)
            {
                preset.Task.MaxWidth = preset.Task.Height;
                preset.Task.MaxHeight = preset.Task.Width;
            }

            return preset;
        }

        private void AddOrUpdateImportedPreset(Preset preset)
        {
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

                MessageBoxResult result = MessageBox.Show(string.Format(Resources.Main_PresetOverwriteWarning, preset.Name), Resources.Overwrite, MessageBoxButton.YesNo, MessageBoxImage.Warning);
                if (result == MessageBoxResult.Yes)
                {
                    this.Update(preset);
                }
            }
            else
            {
                this.Add(preset, false);
            }
        }

        #endregion
    }
}