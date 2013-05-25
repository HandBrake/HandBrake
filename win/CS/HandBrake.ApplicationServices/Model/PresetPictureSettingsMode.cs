// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetPictureSettingsMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Picture Settings Mode when adding presets
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// Picture Settings Mode when adding presets
    /// </summary>
    public enum PresetPictureSettingsMode
    {
        [Display(Name = "None")]
        None = 0,
        [Display(Name = "Custom (Keep AR)")]
        Custom = 1,
        [Display(Name = "Current Source Max Size (Keep AR)")]
        SourceMaximum = 2,
        [Display(Name = "No Limit (Keep AR)")]
        NoLimit = 3,
    }
}