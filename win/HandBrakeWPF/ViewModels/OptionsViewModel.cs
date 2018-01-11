// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The mini view model.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Windows;

    using Caliburn.Micro;
    using HandBrake.Properties;
    using HandBrake.Model;
    using HandBrake.Services.Interfaces;
    using HandBrake.Utilities;
    using HandBrake.ViewModels.Interfaces;
    using PlatformBindings.Models.FileSystem;
    using HandBrake.Utilities.Interfaces;

    public class OptionsViewModel : OptionsViewModelBase
    {
        private readonly IUpdateService updateService;

        private UpdateCheckInformation updateInfo;
        private string updateMessage;
        private bool updateAvailable;
        private bool checkForUpdates;
        private BindingList<string> checkForUpdatesFrequencies = new BindingList<string>();
        private int checkForUpdatesFrequency;
        private int downloadProgressPercentage;

        private string vlcPath;
        private bool minimiseToTray;

        public OptionsViewModel(IUserSettingService userSettingService, IUpdateService updateService, IAboutViewModel aboutViewModel, IErrorService errorService, IDialogService dialogService)
            : base(userSettingService, aboutViewModel, errorService, dialogService)
        {
            this.updateService = updateService;
            this.UpdateMessage = "Click 'Check for Updates' to check for new versions";
        }

        #region Updates

        public bool IsUWP
        {
            get
            {
                return UwpDetect.IsUWP();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether CheckForUpdates.
        /// </summary>
        public bool CheckForUpdates
        {
            get
            {
                return this.checkForUpdates;
            }

            set
            {
                this.checkForUpdates = value;
                this.NotifyOfPropertyChange("CheckForUpdates");
            }
        }

        /// <summary>
        /// Gets or sets CheckForUpdatesFrequencies.
        /// </summary>
        public BindingList<string> CheckForUpdatesFrequencies
        {
            get
            {
                return this.checkForUpdatesFrequencies;
            }

            set
            {
                this.checkForUpdatesFrequencies = value;
                this.NotifyOfPropertyChange("CheckForUpdatesFrequencies");
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether CheckForUpdatesFrequency.
        /// </summary>
        public int CheckForUpdatesFrequency
        {
            get
            {
                return this.checkForUpdatesFrequency;
            }

            set
            {
                this.checkForUpdatesFrequency = value;
                this.NotifyOfPropertyChange("CheckForUpdatesFrequency");
            }
        }

        #endregion Updates

        /// <summary>
        /// Gets or sets a value indicating whether MinimiseToTray.
        /// </summary>
        public bool MinimiseToTray
        {
            get
            {
                return this.minimiseToTray;
            }

            set
            {
                this.minimiseToTray = value;
                this.NotifyOfPropertyChange("MinimiseToTray");
            }
        }

        /// <summary>
        /// Load WPF User Settings
        /// </summary>
        public override void OnLoad()
        {
            base.OnLoad();

            // #############################
            // General
            // #############################

            this.CheckForUpdates = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.UpdateStatus);

            // Days between update checks
            this.checkForUpdatesFrequencies.Clear();
            this.checkForUpdatesFrequencies.Add("Weekly");
            this.checkForUpdatesFrequencies.Add("Monthly");

            this.CheckForUpdatesFrequency = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.DaysBetweenUpdateCheck);
            if (this.CheckForUpdatesFrequency > 1)
            {
                this.CheckForUpdatesFrequency = 1;
            }

            // Minimise to Tray
            this.MinimiseToTray = this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.MainWindowMinimize);

            // #############################
            // Picture Tab
            // #############################

            // VLC Path
            this.VLCPath = this.UserSettingService.GetUserSetting<string>(UserSettingConstants.VLCPath) ?? string.Empty;
        }

        #region VLC

        /// <summary>
        /// Browse VLC Path
        /// </summary>
        public void BrowseVlcPath()
        {
            var properties = new FilePickerProperties();
            properties.FileTypes.Add(".exe");
            var vlcfile = new CoreFileContainer(this.VLCPath);
            properties.SuggestedStorageItem = vlcfile;

            IOService?.Pickers?.PickFile(properties)?.ContinueWith(task =>
            {
                var file = task.Result;
                if (file != null)
                {
                    this.VLCPath = file.Path;
                }
            });
        }

        /// <summary>
        /// Gets or sets VLCPath.
        /// </summary>
        public string VLCPath
        {
            get
            {
                return this.vlcPath;
            }

            set
            {
                this.vlcPath = value;
                this.NotifyOfPropertyChange("VLCPath");
            }
        }

        #endregion VLC
        #region Updating

        /// <summary>
        /// Download Complete Action
        /// </summary>
        /// <param name="info">
        /// The info.
        /// </param>
        private void DownloadComplete(DownloadStatus info)
        {
            this.UpdateAvailable = false;
            this.UpdateMessage = info.WasSuccessful ? Resources.OptionsViewModel_UpdateDownloaded : info.Message;

            if (info.WasSuccessful)
            {
                Process.Start(Path.Combine(Path.GetTempPath(), "handbrake-setup.exe"));
                Execute.OnUIThread(() => Application.Current.Shutdown());
            }
        }

        /// <summary>
        /// Download an Update
        /// </summary>
        public void DownloadUpdate()
        {
            this.UpdateMessage = "Preparing for Update ...";
            this.updateService.DownloadFile(this.updateInfo.DownloadFile, this.updateInfo.Signature, this.DownloadComplete, this.DownloadProgress);
        }

        /// <summary>
        /// Check for updates
        /// </summary>
        public void PerformUpdateCheck()
        {
            this.UpdateMessage = "Checking for Updates ...";
            this.updateService.CheckForUpdates(this.UpdateCheckComplete);
        }

        /// <summary>
        /// Update Check Complete
        /// </summary>
        /// <param name="info">
        /// The info.
        /// </param>
        private void UpdateCheckComplete(UpdateCheckInformation info)
        {
            this.updateInfo = info;
            if (info.NewVersionAvailable)
            {
                this.UpdateMessage = Resources.OptionsViewModel_NewUpdate;
                this.UpdateAvailable = true;
            }
            else if (Environment.Is64BitOperatingSystem && !System.Environment.Is64BitProcess)
            {
                this.UpdateMessage = Resources.OptionsViewModel_64bitAvailable;
                this.UpdateAvailable = true;
            }
            else
            {
                this.UpdateMessage = Resources.OptionsViewModel_NoNewUpdates;
                this.UpdateAvailable = false;
            }
        }

        /// <summary>
        /// Gets or sets UpdateMessage.
        /// </summary>
        public string UpdateMessage
        {
            get
            {
                return this.updateMessage;
            }
            set
            {
                this.updateMessage = value;
                this.NotifyOfPropertyChange(() => this.UpdateMessage);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether UpdateAvailable.
        /// </summary>
        public bool UpdateAvailable
        {
            get
            {
                return this.updateAvailable;
            }
            set
            {
                this.updateAvailable = value;
                this.NotifyOfPropertyChange(() => this.UpdateAvailable);
            }
        }

        /// <summary>
        /// Gets or sets DownloadProgressPercentage.
        /// </summary>
        public int DownloadProgressPercentage
        {
            get
            {
                return this.downloadProgressPercentage;
            }
            set
            {
                this.downloadProgressPercentage = value;
                this.NotifyOfPropertyChange(() => this.DownloadProgressPercentage);
            }
        }

        /// <summary>
        /// Download Progress Action
        /// </summary>
        /// <param name="info">
        /// The info.
        /// </param>
        private void DownloadProgress(DownloadStatus info)
        {
            if (info.TotalBytes == 0 || info.BytesRead == 0)
            {
                this.UpdateAvailable = false;
                this.UpdateMessage = info.WasSuccessful ? Resources.OptionsViewModel_UpdateDownloaded : Resources.OptionsViewModel_UpdateServiceUnavailable;
                return;
            }

            long p = (info.BytesRead * 100) / info.TotalBytes;
            int progress;
            int.TryParse(p.ToString(CultureInfo.InvariantCulture), out progress);
            this.DownloadProgressPercentage = progress;
            this.UpdateMessage = string.Format(
                "Downloading... {0}% - {1}k of {2}k", this.DownloadProgressPercentage, (info.BytesRead / 1024), (info.TotalBytes / 1024));
        }

        #endregion Updating

        protected override void Save()
        {
            base.Save();

            /* General */
            this.UserSettingService.SetUserSetting(UserSettingConstants.UpdateStatus, this.CheckForUpdates);
            this.UserSettingService.SetUserSetting(UserSettingConstants.DaysBetweenUpdateCheck, this.CheckForUpdatesFrequency);
            UserSettingService.SetUserSetting(UserSettingConstants.MainWindowMinimize, this.MinimiseToTray);

            /* Previews */
            this.UserSettingService.SetUserSetting(UserSettingConstants.VLCPath, this.VLCPath);
        }
    }
}