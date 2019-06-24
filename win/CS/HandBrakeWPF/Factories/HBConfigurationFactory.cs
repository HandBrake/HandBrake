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

    using HandBrake.Interop.Model;

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
                                             ScalingMode = UserSettingService.GetUserSetting<VideoScaler>(UserSettingConstants.ScalingMode), 
                                             PreviewScanCount = UserSettingService.GetUserSetting<int>(UserSettingConstants.PreviewScanCount), 
                                             Verbosity = UserSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity), 
                                             MinScanDuration = UserSettingService.GetUserSetting<int>(UserSettingConstants.MinScanDuration), 
                                             SaveLogToCopyDirectory = UserSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory), 
                                             SaveLogWithVideo = UserSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo), 
                                             SaveLogCopyDirectory = UserSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory), 
                                             RemoteServiceEnabled = UserSettingService.GetUserSetting<bool>(UserSettingConstants.RemoteServiceEnabled),
                                             RemoteServicePort = UserSettingService.GetUserSetting<int>(UserSettingConstants.RemoteServicePort),
                                             EnableVceEncoder = UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableVceEncoder),
                                             EnableNvencEncoder = UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableNvencEncoder),
                                             EnableQsvEncoder = UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncEncoding),
                                             EnableQuickSyncDecoding = UserSettingService.GetUserSetting<bool>(UserSettingConstants.EnableQuickSyncDecoding),
                                             UseQSVDecodeForNonQSVEnc = UserSettingService.GetUserSetting<bool>(UserSettingConstants.UseQSVDecodeForNonQSVEnc)
                                         };

            return config;
        }
    }
}
