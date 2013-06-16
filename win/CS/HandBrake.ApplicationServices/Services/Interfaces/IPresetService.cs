// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPresetService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Preset Service Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System.Collections.ObjectModel;

    using HandBrake.ApplicationServices.Model;

    /// <summary>
    /// The Preset Service Interface
    /// </summary>
    public interface IPresetService
    {
        /// <summary>
        /// Gets a Collection of presets.
        /// </summary>
        ObservableCollection<Preset> Presets { get; }

        /// <summary>
        /// Gets DefaultPreset.
        /// </summary>
        Preset DefaultPreset { get; }

        /// <summary>
        /// The load.
        /// </summary>
        void Load();

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
        void Remove(Preset preset);

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
        /// Check if the built in Presets stored are not out of date.
        /// Update them if they are.
        /// </summary>
        /// <returns>true if out of date</returns>
        bool CheckIfPresetsAreOutOfDate();

        /// <summary>
        /// Check if the preset "name" exists in either Presets or UserPresets lists.
        /// </summary>
        /// <param name="name">Name of the preset</param>
        /// <returns>True if found</returns>
        bool CheckIfPresetExists(string name);
    }
}