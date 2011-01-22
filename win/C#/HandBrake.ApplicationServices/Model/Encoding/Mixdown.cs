/*  Mixdown.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System.ComponentModel;

    /// <summary>
    /// The Mixdown Mode
    /// </summary>
    public enum Mixdown
    {
        [Description("Dolby Pro Logic II")]
        DolbyProLogicII = 0,

        [Description("Auto")]
        Auto,

        [Description("Mono")]
        Mono,

        [Description("Stereo")]
        Stereo,

        [Description("Dolby Surround")]
        DolbySurround,

        [Description("6 Channel Discrete")]
        SixChannelDiscrete
    }
}
