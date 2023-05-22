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
    using System.ComponentModel;
    using System.IO;
    using System.Linq;
    using System.Text.Json;
    using System.Windows;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model.Presets;
    using HandBrake.Interop.Interop.Json.Presets;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Presets.Factories;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;

    using GeneralApplicationException = HandBrake.App.Core.Exceptions.GeneralApplicationException;

    public class PresetService : IPresetService
    {
        public const int ForcePresetReset = 3;
        public static string UserPresetCategoryName = "Custom Presets";
        private readonly string presetFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(HandBrakeVersionHelper.IsNightly()), "presets.json");
        private readonly ObservableCollection<IPresetObject> presets = new ObservableCollection<IPresetObject>(); // Can store Presets and PresetDisplayCategory objects.
        private readonly Dictionary<string, Preset> flatPresetDict = new Dictionary<string, Preset>();
        private readonly IErrorService errorService;
        private readonly IUserSettingService userSettingService;
        private ILog log;

        public PresetService(IErrorService errorService, IUserSettingService userSettingService, ILog logService)
        {
            this.log = logService;
            this.errorService = errorService;
            this.userSettingService = userSettingService;
        }

        public event EventHandler PresetCollectionChanged;

        public ObservableCollection<IPresetObject> Presets
        {
            get
            {
                return this.presets;
            }
        }

        public Preset DefaultPreset
        {
            get
            {
                return this.flatPresetDict.Values.FirstOrDefault(p => p.IsDefault);
            }
        }

        public void Load()
        {
            // Load the presets from file
            this.LoadPresets();
        }

        public Preset GetPresetByName(string name)
        {
            Preset foundPreset = this.FindPreset(name);
            if (foundPreset != null)
            {
                return foundPreset;
            }

            return null;
        }

        public Preset GetDefaultPreset()
        {
            if (this.DefaultPreset != null)
            {
                return new Preset(this.DefaultPreset);
            }
            
            return null;
        }

        public string GenerateUniqueName(string currentName)
        {
            bool found = true;
            string newName = string.Format("{0} (1)", currentName);
            int i = 1;

            while (found)
            {
                if (flatPresetDict.ContainsKey(newName))
                {
                    i += 1;
                    newName = string.Format("{0} ({1})", currentName, i);
                }
                else
                {
                    found = false; // Break out the loop, we found a unique. 
                }
            }

            return newName;
        }

        public bool Add(Preset preset)
        {
            return this.Add(preset, false);
        }

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
                    PresetDisplayCategory newCategory = new PresetDisplayCategory(preset.Category, preset.IsBuildIn, new BindingList<Preset> { preset });
                    if (!isLoading && preset.Category == UserPresetCategoryName)
                    {
                        this.presets.Insert(0, newCategory); // Order the user Category up top.
                    }
                    else
                    {
                        this.presets.Add(newCategory);
                    }
                }
                else
                {
                    // Preset has no category. 
                    this.presets.Add(preset);
                }

                this.flatPresetDict.Add(preset.Name, preset);

                // Update the presets file
                if (!isLoading)
                {
                    this.SavePresetFiles();
                    this.OnPresetCollectionChanged();
                }

                return true;
            }
            else
            {
                this.Update(preset.Name, preset);
                this.OnPresetCollectionChanged();
                return true;
            }
        }

        public void Import(string filename)
        {
            if (!string.IsNullOrEmpty(filename))
            {
                PresetTransportContainer container;
                try
                {
                    container = HandBrakePresetService.GetPresetsFromFile(filename);
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
                    bool containsBuildInPreset = false;
                    foreach (var objectPreset in container.PresetList)
                    {
                        HBPresetCategory category = JsonSerializer.Deserialize<HBPresetCategory>(objectPreset.ToString(), JsonSettings.Options);
                        if (category != null && category.ChildrenArray != null && category.ChildrenArray.Count > 0)
                        {
                            foreach (HBPreset hbPreset in category.ChildrenArray)
                            {
                                Preset preset = this.ConvertHbPreset(hbPreset, category.PresetName);
                                preset.IsPresetDisabled = this.IsPresetDisabled(preset);
                                preset.IsDefault = false; // When importing, force the user to reset default manually.  This prevents conflicts.
                                if (!preset.IsBuildIn)
                                {
                                    this.AddOrUpdateImportedPreset(preset);
                                }
                                else
                                {
                                    containsBuildInPreset = true;
                                }
                            }
                        }
                        else
                        {
                            HBPreset hbPreset = JsonSerializer.Deserialize<HBPreset>(objectPreset.ToString(), JsonSettings.Options);
                            if (hbPreset != null)
                            {
                                Preset preset = this.ConvertHbPreset(hbPreset, null);
                                preset.IsDefault = false; // When importing, force the user to reset default manually.  This prevents conflicts.
                                preset.IsPresetDisabled = this.IsPresetDisabled(preset);
                                if (!preset.IsBuildIn)
                                {
                                    this.AddOrUpdateImportedPreset(preset);
                                }
                                else
                                {
                                    containsBuildInPreset = true;
                                }
                            }
                        }
                    }

                    if (containsBuildInPreset)
                    {
                        this.errorService.ShowMessageBox(
                            Properties.Resources.PresetService_ImportingBuiltInWarning,
                            Properties.Resources.Warning,
                            MessageBoxButton.OK,
                            MessageBoxImage.Warning);
                    }
                }
            }
        }

        public void Export(string filename, string presetName)
        {
            Preset foundPreset = this.FindPreset(presetName);
            if (foundPreset != null)
            {
                PresetTransportContainer container = JsonPresetFactory.ExportPreset(foundPreset);
                HandBrakePresetService.ExportPreset(filename, container);
            }
        }

        public void ExportCategories(string filename, IList<PresetDisplayCategory> categories)
        {
            PresetTransportContainer container = JsonPresetFactory.ExportPresetCategories(categories);
            HandBrakePresetService.ExportPreset(filename, container);
        }

        public void Update(string presetName, Preset update)
        {
            Preset preset = this.FindPreset(presetName);
            if (preset != null)
            {
                preset.Task = update.Task;
                preset.Category = update.Category;
                preset.Description = update.Description;
                preset.AudioTrackBehaviours = update.AudioTrackBehaviours;
                preset.SubtitleTrackBehaviours = update.SubtitleTrackBehaviours;

                // Update the presets file
                this.SavePresetFiles();
            }
        }

        public void Replace(string presetName, Preset replacement)
        {
            Preset foundPreset = this.FindPreset(presetName);
            if (foundPreset != null)
            {
                this.Remove(foundPreset, true, true);
                this.Add(replacement, false);
                this.OnPresetCollectionChanged();
            }
        }

        public bool Remove(string presetName)
        {
            Preset foundPreset = this.FindPreset(presetName);
            if (foundPreset != null)
            {
                return this.Remove(foundPreset, false, false);
            }

            return false;
        }

        public void AddCategory(string categoryName)
        {
            if (string.IsNullOrEmpty(categoryName))
            {
                this.errorService.ShowMessageBox(
                    Resources.PresetService_CategoryNameEmpty,
                    Resources.Error,
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
            }

            PresetDisplayCategory category = this.presets.FirstOrDefault(a => a.Category == categoryName) as PresetDisplayCategory;
            if (category != null)
            {
                this.errorService.ShowMessageBox(
                    Resources.PresetService_CategoryAlreadyExists,
                    Resources.Error,
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);

                return;
            }

            // Otherwise, if we have category but it doesn't exist, create it.
            this.presets.Add(new PresetDisplayCategory(categoryName, false, new BindingList<Preset>()));

            // Update the presets file
            this.SavePresetFiles();
        }

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
                    this.flatPresetDict.Remove(preset.Name);
                }

                // Cleanup the category if we can.
                if (category.Presets.Count == 0)
                {
                    this.presets.Remove(category);
                }

                this.SavePresetFiles();
            }
        }

        public void SetDefault(string presetName)
        {
            Preset foundPreset = this.FindPreset(presetName);
            if (foundPreset != null)
            {
                // Set IsDefault false for everything.
                foreach (Preset item in this.flatPresetDict.Values)
                {
                    item.IsDefault = false;
                }

                // Set the new preset to default.
                foundPreset.IsDefault = true;

                this.SavePresetFiles();
            }
        }

        public Preset GetPreset(string name)
        {
            return this.FindPreset(name);
        }

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
                        this.flatPresetDict.Remove(toRemove.Name);
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
                    this.flatPresetDict.Remove(((Preset)item).Name);
                }
            }
        }

        public void ClearAll()
        {
            this.presets.Clear();
        }

        public void UpdateBuiltInPresets()
        {
            // Clear the current built in Presets and now parse the temporary Presets file.
            this.ClearBuiltIn();

            bool hasUserDefault = this.flatPresetDict.Values.FirstOrDefault(f => f.IsDefault) != null;

            IList<HBPresetCategory> presetCategories = HandBrakePresetService.GetBuiltInPresets();

            foreach (var category in presetCategories)
            {
                foreach (var hbpreset in category.ChildrenArray)
                {
                    Preset preset = JsonPresetFactory.ImportPreset(hbpreset);
                    preset.IsBuildIn = true;
                    preset.Category = category.PresetName;
                    preset.IsPresetDisabled = this.IsPresetDisabled(preset) || hbpreset.PresetDisabled;

                    if (hbpreset.Default && hasUserDefault)
                    {
                        preset.IsDefault = false;
                    }

                    this.Add(preset, true);
                }
            }

            // Verify we have presets.
            if (this.presets.Count == 0)
            {
                this.ServiceLogMessage("Failed to load built-in presets.");
                throw new GeneralApplicationException("Failed to load built-in presets.", "Restarting HandBrake may resolve this issue", null);
            }

            // Store the changes to disk
            this.SavePresetFiles();
        }

        public void DeleteBuiltInPresets()
        {
            List<Preset> allPresets = new List<Preset>(this.flatPresetDict.Values);
            foreach (Preset preset in allPresets)
            {
                if (preset.IsBuildIn)
                {
                    if (!preset.IsDefault)
                    {
                        this.Remove(preset, true, false); // Don't remove the default preset.
                    }
                }
            }
        }

        public bool CheckIfPresetExists(string name)
        {
            Preset preset = this.FindPreset(name);
            if (preset != null)
            {
                return true;
            }

            return false;
        }

        public bool CanUpdatePreset(string name)
        {
            Preset preset = this.FindPreset(name);
            if (preset != null)
            {
                return !preset.IsBuildIn;
            }

            return true;
        }

        public void SetSelected(string presetName)
        {
            Preset foundPreset = this.FindPreset(presetName);
            if (foundPreset != null)
            {
                foreach (var item in this.flatPresetDict.Values)
                {
                    item.IsSelected = false;
                }

                foundPreset.IsSelected = true;

                IPresetObject category = this.presets.FirstOrDefault(p => p.Category == foundPreset.Category);
                if (category != null)
                {
                    category.IsExpanded = true;
                }
            }
        }

        public void ChangePresetCategory(string presetName, string categoryName)
        {
            Preset foundPreset = this.FindPreset(presetName);
            if (foundPreset != null)
            {
                if (string.IsNullOrEmpty(categoryName))
                {
                    return;
                }

                foundPreset.Category = categoryName;

                this.Save();
                this.ClearPresetService();
                this.Load();
                this.LoadCategoryStates();
                this.OnPresetCollectionChanged();
            }
        }

        public void SaveCategoryStates()
        {
            List<string> expandedPresets = new List<string>();
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
            List<string> expandedPresets = this.userSettingService.GetUserSetting<List<string>>(UserSettingConstants.PresetExpandedStateList);
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

            foreach (var item in this.presets)
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

        public void Save()
        {
            this.SavePresetFiles();
        }
        
        private string ArchivePresetFile(string file, bool delete)
        {
            try
            {
                // Recover from Error.
                PresetVersion version = HandBrakePresetService.GetCurrentPresetVersion();
                string archiveFile = string.Format("{0}.{1}_{2}_{3}.archive", file, version.Major, version.Minor, version.Micro);
                if (File.Exists(file))
                {
                    int counter = 0;
                    while (File.Exists(archiveFile))
                    {
                        counter = counter + 1;
                        string appendedNumber = string.Format("({0})", counter);
                        archiveFile = string.Format("{0}.{1}_{2}_{3} {4}.archive", file, version.Major, version.Minor, version.Micro, appendedNumber);
                    }

                    File.Copy(file, archiveFile);

                    if (delete)
                    {
                        File.Delete(file);
                    }
                }

                return archiveFile;
            }
            catch (IOException e)
            {
                this.ServiceLogMessage(e.ToString());
            }

            return "Sorry, the archiving failed.";
        }

        private void LoadPresets()
        {
            // First clear the Presets arraylists
            this.presets.Clear();
            bool forceSave = false;

            // Load the presets file.
            try
            {
                // If we don't have a presets file. Create one for first load.
                if (!File.Exists(this.presetFile))
                {
                    this.UpdateBuiltInPresets();
                    return;
                }

                // Otherwise, we already have a file, so lets try load it.
                PresetTransportContainer container = null;
                try
                {
                    container = HandBrakePresetService.GetPresetsFromFile(this.presetFile);

                    PresetVersion presetVersion = HandBrakePresetService.GetCurrentPresetVersion();
                    if (container.VersionMajor != presetVersion.Major || container.VersionMinor != presetVersion.Minor || container.VersionMicro != presetVersion.Micro)
                    {
                        container = HandBrakePresetService.UpgradePresets(this.presetFile);
                        forceSave = true;
                    }
                }
                catch (Exception exc)
                {
                    this.ServiceLogMessage("Corrupted Presets File Detected: " + Environment.NewLine + exc);
                }

                // Sanity Check. Did the container deserialize.
                if (container?.PresetList == null)
                {
                    this.ServiceLogMessage("Attempting Preset Recovery ...");
                    string filename = this.ArchivePresetFile(this.presetFile, true);
                    this.errorService.ShowMessageBox(
                        Resources.PresetService_UnableToLoadPresets + filename,
                        Resources.PresetService_UnableToLoad,
                        MessageBoxButton.OK,
                        MessageBoxImage.Warning);

                    this.UpdateBuiltInPresets();
                    this.ServiceLogMessage("Recovery Completed!");
                    return; // Update built-in presets stores the presets locally, so just return.
                }

                // Force Upgrade of presets
                if (this.userSettingService.GetUserSetting<int>(UserSettingConstants.ForcePresetReset) < ForcePresetReset)
                {
                    this.userSettingService.SetUserSetting(UserSettingConstants.ForcePresetReset, ForcePresetReset);

                    string fileName = this.ArchivePresetFile(this.presetFile, true);
                    this.errorService.ShowMessageBox(
                        Resources.Presets_PresetForceReset
                        + Environment.NewLine + Environment.NewLine + Resources.PresetService_ArchiveFile + fileName,
                        Resources.PresetService_UnableToLoad,
                        MessageBoxButton.OK,
                        MessageBoxImage.Information);
                    this.UpdateBuiltInPresets(); // Update built-in presets stores the presets locally, so just return.
                    return;
                }
                
                this.ProcessPresetList(container);

                CheckAndSetDefault(); // Make a preset default if one we have none.

                if (forceSave)
                {
                    this.SavePresetFiles();
                }
            }
            catch (Exception ex)
            {
                this.ServiceLogMessage(ex.ToString());
                this.ArchivePresetFile(this.presetFile, true);
                this.UpdateBuiltInPresets();
            }
        }

        private void ProcessPresetList(PresetTransportContainer container)
        {
            // The presets file loaded was OK, so process it.
            foreach (var item in container.PresetList)
            {
                object deserializedItem = JsonSerializer.Deserialize<HBPresetCategory>(item.ToString(), JsonSettings.Options);

                // Handle Categorised Presets.
                HBPresetCategory category = deserializedItem as HBPresetCategory;
                if (category != null && category.Folder)
                {
                    foreach (HBPreset hbpreset in category.ChildrenArray)
                    {
                        Preset preset = JsonPresetFactory.ImportPreset(hbpreset);

                        // Migration
                        preset.Category = category.PresetName == "User Presets" ? UserPresetCategoryName : category.PresetName;
                        if (string.IsNullOrEmpty(preset.Category))
                        {
                            // We don't allow presets without a category.
                            preset.Category = UserPresetCategoryName;
                        }
                        preset.IsBuildIn = hbpreset.Type == 0;
                        preset.IsPresetDisabled = this.IsPresetDisabled(preset);

                        this.Add(preset, true);
                    }
                }

                // Uncategorised Presets (Legacy Presets, try upconvert them)
                deserializedItem = JsonSerializer.Deserialize<HBPreset>(item.ToString(), JsonSettings.Options);
                HBPreset hbPreset = deserializedItem as HBPreset;
                if (hbPreset != null && !hbPreset.Folder)
                {
                    Preset preset = JsonPresetFactory.ImportPreset(hbPreset);
                    preset.Category = UserPresetCategoryName;
                    preset.IsBuildIn = hbPreset.Type == 0;
                    preset.IsPresetDisabled = this.IsPresetDisabled(preset);

                    this.Add(preset, true);
                }
            }
        }

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

                // Handle User Presets.
                List<HBPresetCategory> presetCategories = this.HandlePresetListsForSave(this.Presets);

                // Wrap the categories in a container. 
                PresetVersion presetVersion = HandBrakePresetService.GetCurrentPresetVersion();
                PresetTransportContainer container = new PresetTransportContainer(presetVersion.Major, presetVersion.Minor, presetVersion.Micro) { PresetList = new List<object>() };
                container.PresetList.AddRange(presetCategories);

                // Write the preset container out to file.
                using (FileStream strm = new FileStream(this.presetFile, FileMode.Create, FileAccess.Write))
                {
                    string presetsJson = JsonSerializer.Serialize(container, JsonSettings.Options);
                    using (StreamWriter writer = new StreamWriter(strm))
                    {
                        writer.WriteLine(presetsJson);
                    }
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException("Unable to write to the presets file.", "The details section below may indicate why this error has occurred.", exc);
            }
        }

        private void ClearPresetService()
        {
            this.flatPresetDict.Clear();
            this.presets.Clear();
        }

        private List<HBPresetCategory> HandlePresetListsForSave(IList<IPresetObject> processList)
        {
            // Organise the Presets list into Json Equivalent objects.
            List<HBPresetCategory> presetCategoryList = new List<HBPresetCategory>();

            foreach (IPresetObject item in processList)
            {
                PresetDisplayCategory presetDisplayCategory = item as PresetDisplayCategory;
                if (presetDisplayCategory != null)
                {
                    foreach (Preset preset in presetDisplayCategory.Presets)
                    {
                        HBPreset hbPreset = JsonPresetFactory.CreateHbPreset(preset);
                        HBPresetCategory foundCategory = presetCategoryList.FirstOrDefault(c => c.PresetName == item.Category);

                        if (foundCategory != null)
                        {
                            foundCategory.ChildrenArray.Add(hbPreset);
                        }
                        else
                        {
                            presetCategoryList.Add(new HBPresetCategory
                                                              {
                                                                  ChildrenArray = new List<HBPreset> { hbPreset },
                                                                  Folder = true,
                                                                  PresetName = item.Category,
                                                                  Type = preset.IsBuildIn ? 0 : 1
                                                              });
                        }
                    }
                }
            }

            return presetCategoryList;
        }

        private Preset ConvertHbPreset(HBPreset hbPreset, string categoryName)
        {
            Preset preset = JsonPresetFactory.ImportPreset(hbPreset);
            preset.Category = !string.IsNullOrEmpty(categoryName) ? categoryName : UserPresetCategoryName;

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
                    this.errorService.ShowMessageBox(Resources.Main_PresetErrorBuiltInName, Resources.Error, MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }

                MessageBoxResult result = this.errorService.ShowMessageBox(string.Format(Resources.Main_PresetOverwriteWarning, preset.Name), Resources.Overwrite, MessageBoxButton.YesNo, MessageBoxImage.Warning);
                if (result == MessageBoxResult.Yes)
                {
                    this.Update(preset.Name, preset);
                }
            }
            else
            {
                this.Add(preset, false);
            }
        }

        private bool IsPresetDisabled(Preset preset)
        {
            if (preset.Task.VideoEncoder == null || !HandBrakeEncoderHelpers.VideoEncoders.Contains(preset.Task.VideoEncoder))
            {
                return true;
            }

            return false;
        }

        private void OnPresetCollectionChanged()
        {
            this.PresetCollectionChanged?.Invoke(this, EventArgs.Empty);
        }

        private void CheckAndSetDefault()
        {
            if (this.DefaultPreset == null)
            {
                if (this.presets.Count == 0)
                {
                    this.UpdateBuiltInPresets(); // We don't allow no presets.
                }

                Preset p = this.flatPresetDict.Values.FirstOrDefault();
                if (p != null)
                {
                    p.IsDefault = true;
                }
                
                this.Save();
            }
        }

        private bool Remove(Preset preset, bool overrideDefaultCheck, bool skipCategoryRemoval)
        {
            if (preset == null)
            {
                return false;
            }

            if (preset.IsDefault && !overrideDefaultCheck)
            {
                return false;
            }

            PresetDisplayCategory category = this.presets.FirstOrDefault(p => p.Category == preset.Category) as PresetDisplayCategory;
            if (category != null)
            {
                // Remove the preset, and cleanup the category if it's not got any presets in it.
                category.Presets.Remove(preset);
                this.flatPresetDict.Remove(preset.Name);
                if (category.Presets.Count == 0 && !skipCategoryRemoval)
                {
                    this.presets.Remove(category);
                }
            }
            else
            {
                this.presets.Remove(preset);
                this.flatPresetDict.Remove(preset.Name);
            }

            this.SavePresetFiles();
            this.OnPresetCollectionChanged();

            return true;
        }

        private void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("Preset Service: {0}{1}{0}", Environment.NewLine, message));
        }

        private Preset FindPreset(string presetName)
        {
            if (this.flatPresetDict.ContainsKey(presetName))
            {
                return this.flatPresetDict[presetName];
            }

            return null;
        }
    }
}