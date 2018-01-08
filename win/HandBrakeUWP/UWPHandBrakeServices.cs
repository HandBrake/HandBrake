// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UWPHandBrakeServices.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   UWP Services.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeUWP
{
    using HandBrake;
    using HandBrake.Services.Interfaces;
    using HandBrake.Utilities;
    using HandBrake.Utilities.Interfaces;
    using HandBrakeUWP.Utilities;

    /// <summary>
    /// UWP Services.
    /// </summary>
    public class UWPHandBrakeServices : HandBrakeServices
    {
        public UWPHandBrakeServices()
        {
            Current = this;
            this.SystemInfo = new SystemInfo();
            this.Process = new ProcessIdentificationService();
        }

        public override ISystemInfo SystemInfo { get; }

        public override ITaskBarService TaskBar { get; }

        public override ISystemStateService SystemState { get; }

        public override ISoundService Sound { get; }

        public override IDialogService Dialog { get; }

        public override IProcessIdentificationService Process { get; }

        public override ICopyService Clipboard { get; }

        public override ViewManagerBase ViewManager { get; }
    }
}