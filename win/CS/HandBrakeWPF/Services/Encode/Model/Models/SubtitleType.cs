// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Subtitle Type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
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
        [Description("PGS")]
        PGS,
        [Description("Unknown")]
        Unknown,
        [Description("Foreign Audio Search")]
        ForeignAudioSearch, // Special Type for Foreign Audio Search
    }
}