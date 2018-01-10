// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LauncherServiceBase.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using Caliburn.Micro;
    using HandBrake.Services.Interfaces;

    /// <summary>
    /// Functions for Launching things.
    /// </summary>
    public abstract class LauncherServiceBase
    {
        public bool UseSystemDefaultPlayer => this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.DefaultPlayer);

        protected IErrorService ErrorService { get; }

        protected IUserSettingService UserSettingService { get; }

        public LauncherServiceBase(IErrorService errorService, IUserSettingService userSettingService)
        {
            this.ErrorService = errorService;
            this.UserSettingService = userSettingService;
        }

        /// <summary>
        /// Plays the File.
        /// </summary>
        /// <param name="path">File to Play.</param>
        public abstract void PlayFile(string path);

        /// <summary>
        /// Opens the Directory.
        /// </summary>
        /// <param name="path">Directory Path.</param>
        public abstract void OpenDirectory(string path);
    }
}