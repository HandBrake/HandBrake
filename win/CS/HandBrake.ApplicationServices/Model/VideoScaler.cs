// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoScaler.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The different scaling modes available in HandBrake
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    using HandBrake.ApplicationServices.Attributes;

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
