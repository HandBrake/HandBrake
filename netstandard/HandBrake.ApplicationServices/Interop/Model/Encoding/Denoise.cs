// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Denoise.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Denoise type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    using HandBrake.ApplicationServices.Attributes;

    /// <summary>
    /// The denoise.
    /// </summary>
    public enum Denoise
    {
        [ShortName("off")]
        Off = 0,

        [ShortName("hqdn3d")]
        hqdn3d = 1,

        [ShortName("nlmeans")]
        NLMeans = 2,
    }
}
