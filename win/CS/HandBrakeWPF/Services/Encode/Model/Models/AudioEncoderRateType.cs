// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoderRateType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio encoder rate type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The audio encoder rate type.
    /// </summary>
    public enum AudioEncoderRateType
    {
        /// <summary>
        /// The bitrate.
        /// </summary>
        [Display(Name = "Bitrate: ")]
        Bitrate, 

        /// <summary>
        /// The quality.
        /// </summary>
        [Display(Name = "Quality: ")]
        Quality, 
    }
}
