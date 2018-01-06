// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeServices.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Cross Platform Services.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake
{
    using HandBrake.Utilities;
    using HandBrake.Utilities.Interfaces;

    /// <summary>
    /// Cross Platform Services.
    /// </summary>
    public abstract class HandBrakeServices
    {
        /// <summary>
        /// Gets or sets the Current Services Instance.
        /// </summary>
        public static HandBrakeServices Current { get; protected set; }

        /// <summary>
        /// Gets System Information.
        /// </summary>
        public abstract ISystemInfo SystemInfo { get; }

        /// <summary>
        /// Gets the TaskBar Service.
        /// </summary>
        public abstract ITaskBarService TaskBar { get; }

        /// <summary>
        /// Gets the System State Service.
        /// </summary>
        public abstract ISystemStateService SystemState { get; }

        /// <summary>
        /// Gets the Sound Service.
        /// </summary>
        public abstract ISoundService Sound { get; }

        /// <summary>
        /// Gets the Dialog Service.
        /// </summary>
        public abstract IDialogService Dialog { get; }

        /// <summary>
        /// Gets the Process Identification Service.
        /// </summary>
        public abstract IProcessIdentificationService Process { get; }
    }
}