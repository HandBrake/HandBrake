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

    using HandBrake.ApplicationServices.Model.General;
    using HandBrake.ApplicationServices.Services;

    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// Update Check Helper
    /// </summary>
    public class UpdateCheckHelper
    {
        /// <summary>
        /// Handle the Update Check Finishing.
        /// </summary>
        /// <param name="result">
        /// The result.
        /// </param>
        public static void UpdateCheckDoneMenu(IAsyncResult result)
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
                return;
            }
            catch (Exception ex)
            {
                errorService.ShowError("Unable to check for updates", "Please try again later, the update service may currently be down.", ex);
            }
        }
    }
}
