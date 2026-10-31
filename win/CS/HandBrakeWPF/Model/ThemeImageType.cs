// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ThemeImageType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum ThemeImageType
    {
        [DisplayName(typeof(Resources), "ThemeImageType_Theme")]
        Theme,

        [DisplayName(typeof(Resources), "ThemeImageType_Colour")]
        Colour,
    }
}
