// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UpdateCheck.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the UpdateCheck type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum UpdateCheck
    {
        [DisplayName(typeof(Resources), "UpdateCheck_Monthly")]
        Monthly = 0,

        [DisplayName(typeof(Resources), "UpdateCheck_Weekly")]
        Weekly,
    }
}
