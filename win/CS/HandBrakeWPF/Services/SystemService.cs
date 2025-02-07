// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SystemService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Monitor the system health for common problems that will directly impact encodes.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.Timers;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.Utilities;

    public class SystemService : ISystemService
    {
        private readonly IUserSettingService userSettingService;
        private readonly IQueueService queueService;
        private readonly ILog log;

        private Timer pollTimer;

        private bool lowStateHit = false;
        private bool lowPowerPause = false;
        private bool storageLowPause = false;

        public SystemService(IUserSettingService userSettingService, ILog logService, IQueueService queueService)
        {
            this.log = logService;
            this.queueService = queueService;
            this.userSettingService = userSettingService;
        }

        public void Start()
        {
            if (this.pollTimer == null)
            {
                this.pollTimer = new Timer();
                this.pollTimer.Interval = 10000; // Check every 10 seconds. 
                this.pollTimer.Elapsed += (o, e) =>
                {
                    this.CheckSystem();
                };

                this.pollTimer.Start();
            }
        }

        private void CheckSystem()
        {
            this.PowerCheck();
            this.StorageCheck();
        }

        private void StorageCheck()
        {
            foreach (string directory in this.queueService.GetActiveJobDestinationDirectories())
            {
                this.CheckDiskSpaceForDirectory(directory);
            }
        }

        private void CheckDiskSpaceForDirectory(string directory)
        {
            if (!string.IsNullOrEmpty(directory) && this.queueService.IsEncoding)
            {
                long lowLevel = this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseQueueOnLowDiskspaceLevel);
                if (!this.storageLowPause && this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace) && !DriveUtilities.HasMinimumDiskSpace(directory, lowLevel))
                {
                    this.log.LogMessage(
                        string.Format(
                            Resources.SystemService_LowDiskSpaceLog,
                            lowLevel / 1000 / 1000 / 1000));
                    this.queueService.Pause(true);
                    this.storageLowPause = true;
                }
                else
                {
                    this.storageLowPause = false;
                }
            }
        }

        private void PowerCheck()
        {
            if (!this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseEncodingOnLowBattery))
            {
                return;
            }

            Win32.PowerState state = Win32.PowerState.GetPowerState();

            if (state == null || state.BatteryFlag == Win32.BatteryFlag.NoSystemBattery || state.BatteryFlag == Win32.BatteryFlag.Unknown)
            {
                return; // Only run if we have a battery.
            }

            int lowBatteryLevel = this.userSettingService.GetUserSetting<int>(UserSettingConstants.LowBatteryLevel);

            if (state.ACLineStatus == Win32.ACLineStatus.Offline && state.BatteryLifePercent <= lowBatteryLevel && !this.lowStateHit)
            {
                if (this.queueService.IsEncoding && !this.queueService.IsPaused)
                {
                    this.lowPowerPause = true;
                    this.queueService.Pause(true);
                }

                Win32.AllowSleep();

                this.ServiceLogMessage(string.Format(Resources.SystemService_LowBatteryLog, state.BatteryLifePercent));
                this.lowStateHit = true;
            }

            // Reset the flags when we start charging. 
            if (state.ACLineStatus == Win32.ACLineStatus.Online)
            {
                if (this.lowPowerPause && this.queueService.IsPaused)
                {
                    this.queueService.Start();
                    this.ServiceLogMessage(string.Format(Resources.SystemService_ACMains, state.BatteryLifePercent));
                }

                this.lowPowerPause = false;
                this.lowStateHit = false;
            }
        }

        private void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("{0}# {1}{0}", Environment.NewLine, message));
        }
    }
}
