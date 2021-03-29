// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PadColour.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum PadColour
    {
        [DisplayName(typeof(Resources), "PadColour_Black")]
        [ShortName("black")]
        Black = 0,

        [DisplayName(typeof(Resources), "PadColour_White")]
        [ShortName("white")]
        White,

        [DisplayName(typeof(Resources), "PadColour_Custom")]
        [ShortName("custom")]
        Custom,
    }
}