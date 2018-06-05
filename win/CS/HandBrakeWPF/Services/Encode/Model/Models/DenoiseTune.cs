// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DenoiseTune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the DenoiseTune type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    /// The denoise tune.
    /// </summary>
    public enum DenoiseTune
    {
        [DisplayName("None")]
        [ShortName("none")]
        None = 0,

        [DisplayName("Film")]
        [ShortName("film")]
        Film,

        [DisplayName("Grain")]
        [ShortName("grain")]
        Grain,

        [DisplayName("High Motion")]
        [ShortName("highmotion")]
        HighMotion,

        [DisplayName("Animation")]
        [ShortName("animation")]
        Animation,

        [DisplayName("Tape")]
        [ShortName("tape")]
        Tape,

        [DisplayName("Sprite")]
        [ShortName("sprite")]
        Sprite,
    }
}
