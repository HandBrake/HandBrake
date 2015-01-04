// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FramerateMode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Mode of Video Encoding. CFR, VFR, PFR
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// The Mode of Video Encoding. CFR, VFR, PFR
    /// </summary>
    public enum FramerateMode
    {
        CFR = 0,
        PFR,
        VFR
    }
}
