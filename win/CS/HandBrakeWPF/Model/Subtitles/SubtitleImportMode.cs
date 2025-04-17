// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IsDefaultModes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Subtitles
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum SubtitleImportMode
    {
        [DisplayName(typeof(Resources), "SubtitleImportMode_None")]
        None = 0,

        [DisplayName(typeof(Resources), "SubtitleImportMode_AllBefore")]
        ImportAllBefore,

        [DisplayName(typeof(Resources), "SubtitleImportMode_AllAfter")]
        ImportAllAfter,
    }
}
