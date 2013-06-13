// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AppStyleHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AppStyleHelper type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The AppStyle Helper.
    /// </summary>
    public class AppStyleHelper
    {
        /// <summary>
        /// Gets a value indicating whether use system colours.
        /// </summary>
        public static bool UseSystemColours
        {
            get
            {
                IUserSettingService userSettingService = IoC.Get<IUserSettingService>();
                bool useSystemColours = userSettingService.GetUserSetting<bool>(UserSettingConstants.UseSystemColours);

                return useSystemColours || SystemParameters.HighContrast;
            }
        }
    }
}
