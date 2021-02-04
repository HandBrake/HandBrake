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

    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrakeWPF.Services.Interfaces;

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
        /// <returns>
        /// The <see cref="HBConfiguration"/>.
        /// </returns>
        public static HBConfiguration Create()
        {
            HBConfiguration config = new HBConfiguration
            {
                PreviewScanCount = UserSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount),
                EnableQuickSyncDecoding = UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncDecoding),
                UseQSVDecodeForNonQSVEnc = UserSettingService.GetUserSetting<bool>(UserSettingConstants.UseQSVDecodeForNonQSVEnc),
                EnableQsvLowPower = UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncLowPower)
            };

            return config;
        }
    }
}
