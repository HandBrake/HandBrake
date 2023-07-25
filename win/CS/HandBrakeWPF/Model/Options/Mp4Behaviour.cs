// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Mp4Behaviour.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Mp4Behaviour type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum Mp4Behaviour
    {
        [DisplayName(typeof(Resources), "Mp4Behaviour_UseMp4")]
        MP4,

        [DisplayName(typeof(Resources), "Mp4Behaviour_UseM4v")]
        M4V,
    }
}

