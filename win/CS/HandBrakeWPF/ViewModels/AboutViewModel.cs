// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AboutViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The About View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System.ComponentModel.Composition;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The About View Model
    /// </summary>
    [Export(typeof(IAboutViewModel))]
    public class AboutViewModel : ViewModelBase, IAboutViewModel
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AboutViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public AboutViewModel(IWindowManager windowManager, IUserSettingService userSettingService)
        {
            this.Title = "About HandBrake";
        }

        /// <summary>
        /// Gets Version.
        /// </summary>
        public string Version
        {
            get
            {
                string nightly = UserSettingService.GetUserSetting<string>(ASUserSettingConstants.HandBrakeVersion).Contains("svn") ? " (SVN / Nightly Build)" : string.Empty;
                return string.Format(
                    "{0} ({1}) {2}",
                    UserSettingService.GetUserSetting<string>(ASUserSettingConstants.HandBrakeVersion),
                    UserSettingService.GetUserSetting<int>(ASUserSettingConstants.HandBrakeBuild),
                    nightly);
            }
        }

        /// <summary>
        /// Close this window.
        /// </summary>
        public void Close()
        {
            this.TryClose();
        }
    }
}
