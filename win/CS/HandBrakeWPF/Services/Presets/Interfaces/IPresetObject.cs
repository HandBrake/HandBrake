// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPresetObject.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Preset Service Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Interfaces
{
    public interface IPresetObject
    {
        bool IsExpanded { get; set; }
        bool IsSelected { get; set; }
        string Category { get; }
    }
}