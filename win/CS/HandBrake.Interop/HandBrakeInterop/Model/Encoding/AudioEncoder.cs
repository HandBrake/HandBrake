// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio encoder enumeration
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// The audio encoder.
    /// </summary>
    public enum AudioEncoder
    {
        [Display(Name = "AAC (faac)")]
        Faac = 0,

        [Display(Name = "AAC (ffmpeg)")]
        ffaac,

        [Display(Name = "MP3 (lame)")]
        Lame,

        [Display(Name = "AC3 (ffmpeg)")]
        Ac3,

        [Display(Name = "Auto Passthru")]
        Passthrough,

        [Display(Name = "AC3 Passthru")]
        Ac3Passthrough,

        [Display(Name = "DTS Passthru")]
        DtsPassthrough,

        [Display(Name = "DTS-HD Passthru")]
        DtsHDPassthrough,

        [Display(Name = "AAC Passthru")]
        AacPassthru,

        [Display(Name = "MP3 Passthru")]
        Mp3Passthru,

        [Display(Name = "Vorbis (vorbis)")]
        Vorbis,

        [Display(Name = "Flac (ffmpeg)")]
        ffflac,
    }
}
