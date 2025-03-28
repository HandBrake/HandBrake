// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ForcedModes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Subtitles
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum ForcedModes
    {
        [DisplayName(typeof(Resources), "ForcedModes_No")]
        [ShortName("none")]
        No = 0,

        [DisplayName(typeof(Resources), "ForcedModes_Yes")]
        [ShortName("yes")]
        Yes,
    }
}