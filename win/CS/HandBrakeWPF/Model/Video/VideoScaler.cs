// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoScaler.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The different scaling modes available in HandBrake
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Video
{
    using HandBrake.Interop.Attributes;

    /// <summary>
    ///  The different scaling modes available in HandBrake
    /// </summary>
    public enum VideoScaler
    {
        [DisplayName("Lanczos (default)")]
        [ShortName("swscale")]
        Lanczos = 0,
    }
}
