// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OutputFormat.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Output format.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode.Model.Models
{
    using System.ComponentModel;
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The Output format.
    /// </summary>
    public enum OutputFormat
    {
        [Description("MP4")]
        [Display(Name = "MP4")]
        Mp4 = 0,

        [Description("MKV")]
        [Display(Name = "MKV")]
        Mkv,
    }
}
