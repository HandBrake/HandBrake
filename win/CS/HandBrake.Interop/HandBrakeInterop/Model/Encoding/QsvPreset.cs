// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QsvPreset.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The qsv preset.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The qsv preset.
    /// </summary>
    public enum QsvPreset
    {
        [Display(Name = "Best Speed")]
        Speed,

        [Display(Name = "Balanced")]
        Balanced,

        [Display(Name = "Best Quality")]
        Quality,
    }
}
