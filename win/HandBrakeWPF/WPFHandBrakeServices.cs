// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WPFHandBrakeServices.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   WPF Services.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake
{
    using HandBrake;
    using HandBrake.Services.Interfaces;
    using HandBrake.Utilities;
    using HandBrake.Utilities.Interfaces;
    using HandBrake.Services;
    using HandBrake.Utilities;

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
            Clipboard = new CopyService();
            ViewManager = new ViewManager();
            NotificationManager = new NotificationService();
        }

        public override ISystemInfo SystemInfo { get; }

        public override ITaskBarService TaskBar { get; }

        public override ISystemStateService SystemState { get; }

        public override ISoundService Sound { get; }

        public override IDialogService Dialog { get; }

        public override IProcessIdentificationService Process { get; }

        public override ICopyService Clipboard { get; }

        public override ViewManagerBase ViewManager { get; }

        public override INotificationService NotificationManager { get; }
    }
}