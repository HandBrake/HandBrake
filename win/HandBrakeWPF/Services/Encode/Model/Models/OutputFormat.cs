﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OutputFormat.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Output format.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using HandBrake.ApplicationServices.Attributes;
    
    /// <summary>
    /// The Output format.
    /// </summary>
    public enum OutputFormat
    {
        [DisplayName("MP4")]
        [ShortName("av_mp4")]
        Mp4 = 0,

        [DisplayName("MKV")]
        [ShortName("av_mkv")]
        Mkv,
    }
}
