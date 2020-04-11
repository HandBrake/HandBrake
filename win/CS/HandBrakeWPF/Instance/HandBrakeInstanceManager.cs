// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeInstanceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hand brake instance manager.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance
{
    using System;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;

    /// <summary>
    /// The HandBrake Instance manager.
    /// Only supports scanning right now.
    /// </summary>
    public static class HandBrakeInstanceManager
    {
        private static IEncodeInstance encodeInstance;
        private static HandBrakeInstance scanInstance;
        private static HandBrakeInstance previewInstance;
        private static bool noHardware;

        public static void Init(bool noHardwareMode)
        {
            noHardware = noHardwareMode;
            HandBrakeUtils.RegisterLogger();
            HandBrakeUtils.EnsureGlobalInit(noHardwareMode);
        }

        public static IEncodeInstance GetEncodeInstance(int verbosity, HBConfiguration configuration, ILog logService, IUserSettingService userSettingService)
        {
            if (!HandBrakeUtils.IsInitialised())
            {
                throw new Exception("Please call Init before Using!");
            }

            if (encodeInstance != null)
            {
                encodeInstance.Dispose();
                encodeInstance = null;
            }

            IEncodeInstance newInstance;

            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.RemoteServiceEnabled))
            {
                newInstance = new RemoteInstance(configuration, logService, userSettingService);
            }
            else
            {
                newInstance = new HandBrakeInstance();
            }

            newInstance.Initialize(verbosity, noHardware);

            encodeInstance = newInstance;

            HandBrakeUtils.SetDvdNav(!userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav));

            return encodeInstance;
        }

        /// <summary>
        /// Gets the scanInstance.
        /// </summary>
        /// <param name="verbosity">
        /// The verbosity.
        /// </param>
        /// <returns>
        /// The <see cref="IHandBrakeInstance"/>.
        /// </returns>
        public static IHandBrakeInstance GetScanInstance(int verbosity)
        {
            if (!HandBrakeUtils.IsInitialised())
            {
                throw new Exception("Please call Init before Using!");
            }

            if (scanInstance != null)
            {
                scanInstance.Dispose();
                scanInstance = null;
            }

            HandBrakeInstance newInstance = new HandBrakeInstance();
            newInstance.Initialize(verbosity, noHardware);
            scanInstance = newInstance;

            return scanInstance;
        }

        /// <summary>
        /// The get encode instance.
        /// </summary>
        /// <param name="verbosity">
        /// The verbosity.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <returns>
        /// The <see cref="IHandBrakeInstance"/>.
        /// </returns>
        public static IHandBrakeInstance GetPreviewInstance(int verbosity, IUserSettingService userSettingService)
        {
            if (!HandBrakeUtils.IsInitialised())
            {
                throw new Exception("Please call Init before Using!");
            }

            if (previewInstance != null)
            {
                previewInstance.Dispose();
                previewInstance = null;
            }

            HandBrakeInstance newInstance = new HandBrakeInstance();
            newInstance.Initialize(verbosity, noHardware);
            previewInstance = newInstance;

            HandBrakeUtils.SetDvdNav(!userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav));

            return previewInstance;
        }
    }
}
