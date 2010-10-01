/*  SubtitleType.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model
{
    using System.ComponentModel;

    /// <summary>
    /// Subtitle Type. 
    /// </summary>
    public enum SubtitleType
    {
        [Description("SSA")]
        SSA,
        [Description("SRT")]
        SRT,
        [Description("VobSub")]
        VobSub,
        [Description("CC")]
        CC,
        [Description("UTF8")]
        UTF8Sub,
        [Description("TX3G")]
        TX3G,
        [Description("Unknown")]
        Unknown
    }
}