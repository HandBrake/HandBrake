// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewModelFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The View Model Factory
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Factories
{
    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The View Model Factory
    /// </summary>
    public class ViewModelFactory
    {
        /// <summary>
        /// The Window Manager
        /// </summary>
        private readonly IWindowManager windowManager;

        /// <summary>
        /// The User Setting Service
        /// </summary>
        private readonly IUserSettingService userSettingsService;

        /// <summary>
        /// Initializes a new instance of the <see cref="ViewModelFactory"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window Manager.
        /// </param>
        /// <param name="userSettingsService">
        /// The user Settings Service.
        /// </param>
        public ViewModelFactory(IWindowManager windowManager, IUserSettingService userSettingsService)
        {
            this.windowManager = windowManager;
            this.userSettingsService = userSettingsService;
        }
    }
}
