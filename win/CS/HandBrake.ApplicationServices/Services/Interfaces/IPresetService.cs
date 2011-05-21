/*  IPresetService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

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
        /// Reads the CLI's CLI output format and load's them into the preset List Preset
        /// </summary>
        /// <param name="cliPath">
        /// The Path to the CLI, leave blank for current folder.
        /// </param>
        void UpdateBuiltInPresets(string cliPath);

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