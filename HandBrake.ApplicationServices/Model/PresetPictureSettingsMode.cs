/*  PresetPictureSettingsMode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// Picture Settings Mode when adding presets
    /// </summary>
    public enum PresetPictureSettingsMode
    {
        [Display(Name = "None")]
        None,
        [Display(Name = "Custom")]
        Custom,
        [Display(Name = "Source Maximum")]
        SourceMaximum,
    }
}