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
    using System.Runtime.CompilerServices;
    using System.Timers;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Interfaces;
    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Logging.Model;
    using HandBrakeWPF.Utilities;

    using Ookii.Dialogs.Wpf;

    public class SystemService : ISystemService
    {
        private readonly IUserSettingService userSettingService;
        private readonly IEncode encodeService;
        private readonly ILog log = LogService.GetLogger();
        private Timer pollTimer;

        private bool criticalStateHit = false;
        private bool lowStateHit = false;
        private bool lowPowerPause = false;
        private bool storageLowPause = false;

        public SystemService(IUserSettingService userSettingService, IEncode encodeService)
        {
            this.userSettingService = userSettingService;
            this.encodeService = encodeService;
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
            string directory = this.encodeService.GetActiveJob()?.Destination;
            if (!string.IsNullOrEmpty(directory)  && this.encodeService.IsEncoding)
            {
                long lowLevel = this.userSettingService.GetUserSetting<long>(UserSettingConstants.PauseEncodeOnLowDiskspaceLevel);
                if (!this.storageLowPause && this.userSettingService.GetUserSetting<bool>(UserSettingConstants.PauseOnLowDiskspace) && !DriveUtilities.HasMinimumDiskSpace(directory, lowLevel))
                {
                    LogService.GetLogger().LogMessage(
                        string.Format(
                            Resources.SystemService_LowDiskSpaceLog,
                            lowLevel / 1000 / 1000 / 1000),
                        LogMessageType.Application,
                        LogLevel.Info);
                    this.encodeService.Pause();
                    this.storageLowPause = true;
                }
            }
        }

        private void PowerCheck()
        {
            Win32.PowerState state = Win32.PowerState.GetPowerState();

            if (state == null || state.BatteryFlag == Win32.BatteryFlag.NoSystemBattery || state.BatteryFlag == Win32.BatteryFlag.Unknown)
            {
                return; // Only run if we have a battery.
            }

            if (state.ACLineStatus == Win32.ACLineStatus.Offline && state.BatteryFlag == Win32.BatteryFlag.Low && !this.lowStateHit)
            {
                if (this.encodeService.IsEncoding && !this.encodeService.IsPasued)
                {
                    this.lowPowerPause = true;
                    this.encodeService.Pause();
                }

                Win32.AllowSleep();

                this.ServiceLogMessage(string.Format(Resources.SystemService_LowBatteryLog, state.BatteryLifePercent));
                this.lowStateHit = true;
            }

            if (state.ACLineStatus == Win32.ACLineStatus.Offline && state.BatteryFlag == Win32.BatteryFlag.Critical && !this.criticalStateHit)
            {
                if (this.encodeService.IsEncoding && !this.encodeService.IsPasued)
                {
                    this.lowPowerPause = true;
                    this.encodeService.Pause(); // In case we missed the low state!
                }

                Win32.AllowSleep();

                this.ServiceLogMessage(string.Format(Resources.SystemService_CriticalBattery, state.BatteryLifePercent));
                this.criticalStateHit = true;
            }

            // Reset the flags when we start charging. 
            if (state.ACLineStatus == Win32.ACLineStatus.Online && state.BatteryFlag >= Win32.BatteryFlag.Low)
            {
                if (this.lowPowerPause && this.encodeService.IsPasued)
                {
                    this.encodeService.Resume();
                    this.ServiceLogMessage(string.Format(Resources.SystemService_ACMains, state.BatteryLifePercent));
                }

                this.lowPowerPause = false;
                this.criticalStateHit = false;
                this.lowStateHit = false;
            }
        }

        private void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("{0}# {1}{0}", Environment.NewLine, message), LogMessageType.Application, LogLevel.Info);
        }
    }
}
