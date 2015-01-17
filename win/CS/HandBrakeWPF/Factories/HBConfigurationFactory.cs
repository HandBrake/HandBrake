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
                                             IsDvdNavDisabled = UserSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav), 
                                             DisableQuickSyncDecoding = UserSettingService.GetUserSetting<bool>(UserSettingConstants.DisableQuickSyncDecoding), 
                                             EnableDxva = UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableDxva), 
                                             ScalingMode = UserSettingService.GetUserSetting<VideoScaler>(UserSettingConstants.ScalingMode), 
                                             PreviewScanCount = UserSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount), 
                                             Verbosity = UserSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity), 
                                             MinScanDuration = UserSettingService.GetUserSetting<int>(UserSettingConstants.MinScanDuration), 
                                             ProcessPriority = UserSettingService.GetUserSetting<string>(UserSettingConstants.ProcessPriority), 
                                             SaveLogToCopyDirectory = UserSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory), 
                                             SaveLogWithVideo = UserSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo), 
                                             SaveLogCopyDirectory = UserSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory), 
                                         };

            return config;
        }
    }
}
