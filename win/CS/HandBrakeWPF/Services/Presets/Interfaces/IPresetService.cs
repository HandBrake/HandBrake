// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPresetService.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Preset Service Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Interfaces
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The Preset Service Interface
    /// </summary>
    public interface IPresetService
    {
        /// <summary>
        /// Subscribe to get notifications when a preset is added.
        /// </summary>
        event EventHandler PresetCollectionChanged;

        /// <summary>
        /// Gets a Collection of presets.
        /// </summary>
        ObservableCollection<IPresetObject> Presets { get; }

        /// <summary>
        /// The load.
        /// </summary>
        void Load();

        /// <summary>
        /// Force save updates to the preset files. Rarely should need to be called. Only used by the preset manager.
        /// </summary>
        void Save();

        /// <summary>
        /// Save the state of the Preset Treeview
        /// </summary>
        void SaveCategoryStates();

        /// <summary>
        /// Load the state of the Preset Treeview.
        /// </summary>
        void LoadCategoryStates();

        /// <summary>
        /// Get a list of preset categories.
        /// </summary>
        /// <param name="userCategoriesOnly">
        /// The user Categories Only.
        /// </param>
        /// <returns>
        /// String list.
        /// </returns>
        IList<PresetDisplayCategory> GetPresetCategories(bool userCategoriesOnly);

        Preset GetPresetByName(string presetName);

        Preset GetDefaultPreset();

        /// <summary>
        /// Generate a new preset name that is not used, based on the current name
        /// </summary>
        string GenerateUniqueName(string currentName);

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
        bool Add(Preset preset);

        /// <summary>
        /// The import.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        void Import(string filename);

        /// <summary>
        /// The export.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <param name="presetName">
        /// The preset.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        void Export(string filename, string presetName);

        void ExportCategories(string filename, IList<PresetDisplayCategory> categories);

        /// <summary>
        /// Update a preset
        /// </summary>
        /// <param name="presetName">
        /// The preset Name.
        /// </param>
        /// <param name="update">
        /// The updated preset
        /// </param>
        void Update(string presetName, Preset update);

        /// <summary>
        /// Remove a preset with a given name from either the built in or user preset list.
        /// </summary>
        /// <param name="presetName">
        /// The Preset to remove
        /// </param>
        /// <returns>
        /// True if it was removed successfully, false otherwise.
        /// </returns>
        bool Remove(string presetName);

        /// <summary>
        /// Remove a group of presets by category
        /// </summary>
        /// <param name="category">
        /// The Category to remove
        /// </param>
        void RemoveGroup(string category);

        /// <summary>
        /// Get a Preset
        /// </summary>
        /// <param name="name">
        /// The name of the preset to get
        /// </param>
        /// <returns>
        /// A Preset or null object
        /// </returns>
        Preset GetPreset(string name);

        /// <summary>
        /// Clear Built-in Presets
        /// </summary>
        void ClearBuiltIn();

        /// <summary>
        /// Clear all presets
        /// </summary>
        void ClearAll();

        /// <summary>
        /// Set Default Preset
        /// </summary>
        /// <param name="presetName">
        /// The name.
        /// </param>
        void SetDefault(string presetName);

        /// <summary>
        /// Returns a value if the preset can be updated / resaved
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <returns>
        /// True if it's not a built-in preset, false otherwise.
        /// </returns>
        bool CanUpdatePreset(string name);

        /// <summary>
        /// Reads the CLI's CLI output format and load's them into the preset List Preset
        /// </summary>
        void UpdateBuiltInPresets();

        /// <summary>
        /// Remove all built-in presets that are not default.
        /// </summary>
        void DeleteBuiltInPresets();

        /// <summary>
        /// Check if the preset "name" exists in either Presets or UserPresets lists.
        /// </summary>
        /// <param name="name">Name of the preset</param>
        /// <returns>True if found</returns>
        bool CheckIfPresetExists(string name);

        /// <summary>
        /// Replace an existing preset with a modified one.
        /// </summary>
        /// <param name="presetName">
        /// The existing preset name
        /// </param>
        /// <param name="replacement">
        /// The replacement.
        /// </param>
        void Replace(string presetName, Preset replacement);

        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="presetName">The preset we want to select.</param>
        void SetSelected(string presetName);

        /// <summary>
        /// Change the category to which a preset belongs.
        /// </summary>
        /// <param name="presetName">
        /// The preset.
        /// </param>
        /// <param name="categoryName">
        /// The category Name.
        /// </param>
        void ChangePresetCategory(string presetName, string categoryName);

        void MoveToTopOfGroup(Preset preset);
        void MoveToBottomOfGroup(Preset preset);
        void MoveUp(Preset preset);
        void MoveDown(Preset preset);
    }
}