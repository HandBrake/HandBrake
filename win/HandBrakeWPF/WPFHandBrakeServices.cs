// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WPFHandBrakeServices.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   WPF Services.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF
{
    using HandBrake;
    using HandBrake.Utilities;
    using HandBrake.Utilities.Interfaces;
    using HandBrakeWPF.Services;
    using HandBrakeWPF.Utilities;

    public class WPFHandBrakeServices : HandBrakeServices
    {
        public WPFHandBrakeServices()
        {
            Current = this;
            SystemInfo = new SystemInfo();
            TaskBar = new TaskBarService();
            SystemState = new SystemStateService();
            Sound = new SoundService();
            Dialog = new DialogService();
            Process = new ProcessIdentificationService();
        }

        public override ISystemInfo SystemInfo { get; }

        public override ITaskBarService TaskBar { get; }

        public override ISystemStateService SystemState { get; }

        public override ISoundService Sound { get; }

        public override IDialogService Dialog { get; }

        public override IProcessIdentificationService Process { get; }
    }
}