// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ServiceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Tempory Class which manages services until Windosor is added back into the project to handle it for us.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices
{
    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop;

    /// <summary>
    /// Tempory Class which manages services until Windosor is added back into the project to handle it for us.
    /// </summary>
    public class ServiceManager
    {
        /// <summary>
        /// The Backing field for HandBrake Instance.
        /// </summary>
        private static HandBrakeInstance handBrakeInstance;

        /// <summary>
        /// Gets UserSettingService.
        /// </summary>
        public static IUserSettingService UserSettingService
        {
            get
            {
                return IoC.Get<IUserSettingService>();
            }
        }

        /// <summary>
        /// Gets HandBrakeInstance.
        /// </summary>
        public static HandBrakeInstance HandBrakeInstance
        {
            get
            {
                return handBrakeInstance ?? (handBrakeInstance = new HandBrakeInstance());
            }
        }
    }
}
