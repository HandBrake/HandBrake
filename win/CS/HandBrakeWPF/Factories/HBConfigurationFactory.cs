// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBConfigurationFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HBConfiguration Factory
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Factories
{
    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// HBConfiguration Factory
    /// </summary>
    public class HBConfigurationFactory
    {
        /// <summary>
        /// The user setting service.
        /// </summary>
        private static readonly IUserSettingService UserSettingService = IoC.Get<IUserSettingService>();

        /// <summary>
        /// The create.
        /// </summary>
        /// <param name="isLoggingEnabled">
        /// The is logging enabled.
        /// </param>
        /// <returns>
        /// The <see cref="HBConfiguration"/>.
        /// </returns>
        public static HBConfiguration Create(bool isLoggingEnabled)
        {
            HBConfiguration config = new HBConfiguration
                                         {
                                             IsDvdNavDisabled = UserSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav),
                                             IsLoggingEnabled = isLoggingEnabled
                                         };

            return config;
        }

        /// <summary>
        /// The create.
        /// </summary>
        /// <returns>
        /// The <see cref="HBConfiguration"/>.
        /// </returns>
        public static HBConfiguration Create()
        {
            return Create(true);
        }
    }
}
