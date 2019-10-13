// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ProcessPriority.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ProcessPriority type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Options
{
    using HandBrake.Interop.Attributes;

    using HandBrakeWPF.Properties;

    public enum ProcessPriority
    {
        [DisplayName(typeof(Resources), "ProcessPriority_High")]
        High = 0,

        [DisplayName(typeof(Resources), "ProcessPriority_AboveNormal")]
        AboveNormal, 

        [DisplayName(typeof(Resources), "ProcessPriority_Normal")]
        Normal,

        [DisplayName(typeof(Resources), "ProcessPriority_BelowNormal")]
        BelowNormal,

        [DisplayName(typeof(Resources), "ProcessPriority_Low")]
        Low
    }
}