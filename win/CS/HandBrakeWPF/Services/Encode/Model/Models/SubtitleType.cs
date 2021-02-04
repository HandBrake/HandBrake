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
    /// Note this must be kept up to date with HandBrake.Interop.Interop.HbLib.hb_subtitle_s_subsource
    /// </summary>
    public enum SubtitleType
    {
        [Description("VobSub")]
        VobSub,

        [Description("CC608SUB")]
        CC608,

        [Description("CC708SUB")]
        CC708,

        [Description("UTF8")]
        UTF8Sub,

        [Description("TX3G")]
        TX3G,

        [Description("SSA")]
        SSA,

        [Description("PGS")]
        PGS,

        [Description("IMPORTED SRT")]
        IMPORTSRT,

        [Description("IMPORTED SSA")]
        IMPORTSSA,
    }
}