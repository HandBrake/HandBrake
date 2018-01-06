// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FramerateMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Mode of Video Encoding. CFR, VFR, PFR
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The Mode of Video Encoding. CFR, VFR, PFR
    /// </summary>
    public enum FramerateMode
    {
        [ShortName("cfr")]
        CFR = 0,

        [ShortName("pfr")]
        PFR,

        [ShortName("vfr")]
        VFR
    }
}
