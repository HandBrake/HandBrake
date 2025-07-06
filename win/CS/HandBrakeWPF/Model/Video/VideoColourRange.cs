// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBMixdown.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb mixdown.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

using HandBrake.Interop.Attributes;
using HandBrakeWPF.Properties;

namespace HandBrakeWPF.Model.Video
{
    /// <summary>
    /// The hb mixdown.
    /// </summary>
    public enum VideoColourRange
    {
        [DisplayName(typeof(Resources), "VideoColourRange_SameAsSource")]
        [ShortName("auto")]
        SameAsSource,

        [DisplayName(typeof(Resources), "VideoColourRange_Limited")]
        [ShortName("limited")]
        Limited,

        [DisplayName(typeof(Resources), "VideoColourRange_Full")]
        [ShortName("full")]
        Full
    }
}