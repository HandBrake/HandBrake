// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Mixdown.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Mixdown type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// The Mixdown mode
    /// </summary>
    public enum Mixdown
    {
        [DisplayString("Dolby Pro Logic II")]
        DolbyProLogicII = 0,

        [DisplayString("Mono")]
        Mono,

        [DisplayString("Stereo")]
        Stereo,

        [DisplayString("Dolby Surround")]
        DolbySurround,

        [DisplayString("6 Channel Discrete")]
        SixChannelDiscrete
    }
}
