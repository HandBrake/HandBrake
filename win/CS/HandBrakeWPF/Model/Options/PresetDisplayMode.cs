// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WhenDone.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the WhenDone type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum PresetDisplayMode
    {
        [DisplayName(typeof(Resources), "PresetDisplayMode_Flat")]
        Flat = 0,

        [DisplayName(typeof(Resources), "PresetDisplayMode_Partial")]
        Partial = 1,

        [DisplayName(typeof(Resources), "PresetDisplayMode_Category")]
        Category = 2
    }
}
