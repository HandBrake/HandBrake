// --------------------------------------------------------------------------------------------------------------------
// <copyright file="hb_subtitle.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.HbLib
{
    internal enum hb_subtitle_s_subsource
    {
        VOBSUB,

        CC608SUB,

        CC708SUB,

        UTF8SUB,

        TX3GSUB,

        SSASUB,

        PGSSUB,

        IMPORTSRT,

        IMPORTSSA
    }
}
