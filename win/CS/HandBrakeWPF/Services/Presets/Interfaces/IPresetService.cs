// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPresetService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Preset Service Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Interfaces
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;

    using HandBrake.Interop.Model;

    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The Preset Service Interface
    /// </summary>
    public interface IPresetService
    {
        /// <summary>
        /// Gets a Collection of presets.
        /// </summary>
        ObservableCollection<IPresetObject> Presets { get; }

        /// <summary>
        /// Gets DefaultPreset.
        /// </summary>
        Preset DefaultPreset { get; }

        /// <summary>
        /// The load.
        /// </summary>
        void Load();

        /// <summary>
        /// Save the state of the Preset Treview
        /// </summary>
        void SaveCategoryStates();

        /// <summary>
        /// Load the state of the Preset Treeview.
        /// </summary>
        void LoadCategoryStates();

        /// <summary>
        /// Get a list of preset categories.
        /// </summary>
        /// <returns>
        /// String list.
        /// </returns>
        IList<PresetDisplayCategory> GetPresetCategories(bool userCategoriesOnly);

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
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        void Export(string filename, Preset preset, HBConfiguration configuration);

        /// <summary>
        /// Update a preset
        /// </summary>
        /// <param name="update">
        /// The updated preset
        /// </param>
        void Update(Preset update);

        /// <summary>
        /// Remove a preset with a given name from either the built in or user preset list.
        /// </summary>
        /// <param name="preset">
        /// The Preset to remove
        /// </param>
        /// <returns>
        /// True if it was removed successfully, false otherwise.
        /// </returns>
        bool Remove(Preset preset);

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
        /// <param name="name">
        /// The name.
        /// </param>
        void SetDefault(Preset name);

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
        /// Check if the preset "name" exists in either Presets or UserPresets lists.
        /// </summary>
        /// <param name="name">Name of the preset</param>
        /// <returns>True if found</returns>
        bool CheckIfPresetExists(string name);

        /// <summary>
        /// Replace an existing preset with a modified one.
        /// </summary>
        /// <param name="existing">
        /// The existing.
        /// </param>
        /// <param name="replacement">
        /// The replacement.
        /// </param>
        void Replace(Preset existing, Preset replacement);

        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="selectedPreset">The preset we want to select.</param>
        void SetSelected(Preset selectedPreset);
    }
}