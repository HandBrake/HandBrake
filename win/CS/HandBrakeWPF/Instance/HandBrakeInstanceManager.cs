﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.Threading;

    using HandBrake.App.Core.Exceptions;
    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Interfaces.Model;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    /// <summary>
    /// The HandBrake Instance manager.
    /// Only supports scanning right now.
    /// </summary>
    public static class HandBrakeInstanceManager
    {
        private static readonly object ProcessingLock = new object();
        private static IEncodeInstance encodeInstance;
        private static HandBrakeInstance scanInstance;
        private static HandBrakeInstance previewInstance;
        private static bool noHardware;
       
        public static void Init(bool noHardwareMode, IUserSettingService userSettingService)
        {
            Thread thread = new Thread(() =>
            {
                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.ForceDisableHardwareSupport))
                {
                    noHardware = true;
                }
                else
                {
                    noHardware = noHardwareMode;
                }

                HandBrakeUtils.RegisterLogger();
                HandBrakeUtils.EnsureGlobalInit(noHardware);
            });
            thread.Start();
            if (!thread.Join(8000))
            {
                // Something is likely handing in a graphics driver.  Force disable this feature so we don't probe the hardware next time around.
                userSettingService.SetUserSetting(UserSettingConstants.ForceDisableHardwareSupport, true);
                throw new GeneralApplicationException(Resources.Startup_UnableToStart, Resources.Startup_UnableToStartInfo);
            }
        }

        public static IEncodeInstance GetEncodeInstance(int verbosity, ILog logService, IUserSettingService userSettingService, IPortService portService)
        {
            lock (ProcessingLock)
            {
                if (!HandBrakeUtils.IsInitialised())
                {
                    throw new Exception("Please call Init before Using!");
                }

                IEncodeInstance newInstance;

                if (userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled)
                    && Portable.IsProcessIsolationEnabled())
                {
                    newInstance = new RemoteInstance(logService, userSettingService, portService);
                }
                else
                {
                    if (encodeInstance != null && !encodeInstance.IsRemoteInstance)
                    {
                        encodeInstance.Dispose();
                        encodeInstance = null;
                    }

                    newInstance = new HandBrakeInstance();
                    HandBrakeUtils.SetDvdNav(
                        !userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav));
                    encodeInstance = newInstance;
                }

                newInstance.Initialize(verbosity, noHardware);
                return newInstance;
            }
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
