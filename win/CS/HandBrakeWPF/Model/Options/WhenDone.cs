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

    public enum WhenDone
    {
        [DisplayName(typeof(Resources), "WhenDone_DoNothing")]
        DoNothing = 0,

        [DisplayName(typeof(Resources), "WhenDone_Shutdown")]
        Shutdown = 1,

        [DisplayName(typeof(Resources), "WhenDone_Suspend")]
        Sleep = 2,

        [DisplayName(typeof(Resources), "WhenDone_Hibernate")]
        Hibernate = 3,

        [DisplayName(typeof(Resources), "WhenDone_LockSystem")]
        LockSystem = 4,

        [DisplayName(typeof(Resources), "WhenDone_Logoff")]
        LogOff = 5,

        [DisplayName(typeof(Resources), "WhenDone_QuitHandBrake")]
        QuickHandBrake = 6,

        [DisplayName(typeof(Resources), "WhenDone_CustomAction")]
        CustomAction = 7,
    }
}
