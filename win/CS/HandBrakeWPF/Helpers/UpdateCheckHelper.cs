// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UpdateCheckHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Update Check Helper
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Model.General;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// Update Check Helper
    /// </summary>
    public class UpdateCheckHelper
    {
        /// <summary>
        /// Perform a startup update check. Abiding by user settings.
        /// </summary>
        public static void PerformStartupUpdateCheck()
        {
            // Make sure it's running on the calling thread
            IUserSettingService userSettingService = IoC.Get<IUserSettingService>();
            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus))
            {
                if (DateTime.Now.Subtract(userSettingService.GetUserSetting<DateTime>(UserSettingConstants.LastUpdateCheckDate)).TotalDays
                    > userSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck))
                {
                    userSettingService.SetUserSetting(UserSettingConstants.LastUpdateCheckDate, DateTime.Now);
                    string url = userSettingService.GetUserSetting<string>(ASUserSettingConstants.HandBrakePlatform).Contains("x86_64")
                                                          ? userSettingService.GetUserSetting<string>(UserSettingConstants.Appcast_x64)
                                                          : userSettingService.GetUserSetting<string>(UserSettingConstants.Appcast_i686);
                    UpdateService.BeginCheckForUpdates(UpdateCheckDone, false,
                        url, userSettingService.GetUserSetting<int>(ASUserSettingConstants.HandBrakeBuild),
                        userSettingService.GetUserSetting<int>(UserSettingConstants.Skipversion));
                }
            }
        }

        /// <summary>
        /// Check for Updates.
        /// </summary>
        public static void CheckForUpdates()
        {
            IUserSettingService userSettingService = IoC.Get<IUserSettingService>();
            userSettingService.SetUserSetting(UserSettingConstants.LastUpdateCheckDate, DateTime.Now);
            string url = userSettingService.GetUserSetting<string>(ASUserSettingConstants.HandBrakePlatform).Contains("x86_64")
                                                  ? userSettingService.GetUserSetting<string>(UserSettingConstants.Appcast_x64)
                                                  : userSettingService.GetUserSetting<string>(UserSettingConstants.Appcast_i686);
            UpdateService.BeginCheckForUpdates(UpdateCheckDoneMenu, false,
                url, userSettingService.GetUserSetting<int>(ASUserSettingConstants.HandBrakeBuild),
                userSettingService.GetUserSetting<int>(UserSettingConstants.Skipversion));
        }

        /// <summary>
        /// Handle the Update Check Finishing.
        /// </summary>
        /// <param name="result">
        /// The result.
        /// </param>
        private static void UpdateCheckDone(IAsyncResult result)
        {
            // Make sure it's running on the calling thread
            IErrorService errorService = IoC.Get<IErrorService>();
            try
            {
                // Get the information about the new build, if any, and close the window
                UpdateCheckInformation info = UpdateService.EndCheckForUpdates(result);

                if (info.NewVersionAvailable)
                {
                    errorService.ShowMessageBox(
                        "A New Update is Available", "Update available!", MessageBoxButton.OK, MessageBoxImage.Information);
                }
            }
            catch (Exception ex)
            {
                errorService.ShowError("Unable to check for updates", "Please try again later, the update service may currently be down.", ex);
            }
        }

        /// <summary>
        /// Handle the Update Check Finishing.
        /// </summary>
        /// <param name="result">
        /// The result.
        /// </param>
        private static void UpdateCheckDoneMenu(IAsyncResult result)
        {
            // Make sure it's running on the calling thread
            IErrorService errorService = IoC.Get<IErrorService>();
            try
            {
                // Get the information about the new build, if any, and close the window
                UpdateCheckInformation info = UpdateService.EndCheckForUpdates(result);

                if (info.NewVersionAvailable)
                {
                    errorService.ShowMessageBox(
                        "A New Update is Available", "Update available!", MessageBoxButton.OK, MessageBoxImage.Information);
                }
                else
                {
                    errorService.ShowMessageBox(
                       "There is no new version at this time.", "No Updates", MessageBoxButton.OK, MessageBoxImage.Information);
                }
            }
            catch (Exception ex)
            {
                errorService.ShowError("Unable to check for updates", "Please try again later, the update service may currently be down.", ex);
            }
        }
    }
}
