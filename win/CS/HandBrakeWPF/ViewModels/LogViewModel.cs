// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Diagnostics;
    using System.Windows;

    using HandBrake.ApplicationServices.Services.Interfaces;
    using Interfaces;

    using HandBrake.ApplicationServices.EventArgs;

    /// <summary>
    /// The Log View Model
    /// </summary>
    public class LogViewModel : ViewModelBase, ILogViewModel
    {
        #region Private Fields

        /// <summary>
        /// Backing field for the encodeService service
        /// </summary>
        private readonly IEncodeServiceWrapper encodeService;

        /// <summary>
        /// Backing field for the Scan Service
        /// </summary>
        private readonly IScanServiceWrapper scanService;

        /// <summary>
        /// The selected tab.
        /// </summary>
        private int selectedTab;

        /// <summary>
        /// The encode log index.
        /// </summary>
        private int encodeLogIndex;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LogViewModel"/> class.
        /// </summary>
        /// <param name="encodeService">
        /// The encode service.
        /// </param>
        /// <param name="scanService">
        /// The scan service.
        /// </param>
        public LogViewModel(IEncodeServiceWrapper encodeService, IScanServiceWrapper scanService)
        {
            this.encodeService = encodeService;
            this.scanService = scanService;
            this.Title = "Log Viewer";
            this.encodeLogIndex = 0;
        }

        /// <summary>
        /// Gets or sets the selected tab.
        /// </summary>
        public int SelectedTab
        {
            get
            {
                return this.selectedTab;
            }
            set
            {
                this.selectedTab = value;
                this.NotifyOfPropertyChange(() => this.SelectedTab);
            }
        }

        /// <summary>
        /// Gets Log.
        /// </summary>
        public string ScanLog
        {
            get
            {
                return this.scanService.ActivityLog;
            }
        }

        /// <summary>
        /// Gets the encodelog.
        /// </summary>
        public string EncodeLog
        {
            get
            {
                return this.encodeService.ActivityLog;
            }
        }

        /// <summary>
        /// Open the Log file directory
        /// </summary>
        public void OpenLogDirectory()
        {
            string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\logs";
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process { StartInfo = { FileName = windir + @"\explorer.exe", Arguments = logDir } };
            prc.Start();
        }

        /// <summary>
        /// Copy the log file to the system clipboard
        /// </summary>
        public void CopyLog()
        {
            Clipboard.SetDataObject(this.SelectedTab == 1 ? this.ScanLog : this.EncodeLog, true);
        }

        /// <summary>
        /// Handle the OnActivate Caliburn Event
        /// </summary>
        protected override void OnActivate()
        {
            this.scanService.ScanCompleted += ScanServiceScanCompleted;
            this.encodeService.EncodeCompleted += EncodeServiceEncodeCompleted;
            this.encodeService.EncodeStatusChanged += this.EncodeServiceEncodeStatusChanged;
            this.scanService.ScanStatusChanged += this.ScanServiceScanStatusChanged;
            this.scanService.ScanStared += this.scanService_ScanStared;
            this.encodeService.EncodeStarted += this.encodeService_EncodeStarted;
            base.OnActivate();

            this.NotifyOfPropertyChange(() => this.ScanLog);
            this.NotifyOfPropertyChange(() => this.EncodeLog);
        }

        /// <summary>
        /// Scan Status has changed, update log window.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ScanServiceScanStatusChanged(object sender, ScanProgressEventArgs e)
        {
            this.NotifyOfPropertyChange(() => this.ScanLog);
        }

        /// <summary>
        /// Encode Status has changed, update log window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeServiceEncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            if (encodeLogIndex != this.encodeService.LogIndex || this.encodeService.LogIndex == -1)
            {
                this.NotifyOfPropertyChange(() => this.EncodeLog);
            }

            encodeLogIndex = this.encodeService.LogIndex;
        }

        /// <summary>
        /// Handle the OnDeactivate Caliburn Event
        /// </summary>
        /// <param name="close">
        /// The close.
        /// </param>
        protected override void OnDeactivate(bool close)
        {
            this.scanService.ScanCompleted -= ScanServiceScanCompleted;
            this.encodeService.EncodeCompleted -= EncodeServiceEncodeCompleted;
            this.encodeService.EncodeStatusChanged -= this.EncodeServiceEncodeStatusChanged;
            this.scanService.ScanStatusChanged -= this.ScanServiceScanStatusChanged;
            this.scanService.ScanStared -= this.scanService_ScanStared;
            this.encodeService.EncodeStarted -= this.encodeService_EncodeStarted;

            base.OnDeactivate(close);
        }

        /// <summary>
        /// Scan Completed Event Handler.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ScanServiceScanCompleted(object sender, ScanCompletedEventArgs e)
        {
            this.NotifyOfPropertyChange(() => this.ScanLog);
        }

        /// <summary>
        /// Encode Completed Event Handler.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeServiceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.NotifyOfPropertyChange(() => this.EncodeLog);
        }

        /// <summary>
        /// The encode service encode started.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void encodeService_EncodeStarted(object sender, EventArgs e)
        {
            this.SelectedTab = 0;
        }

        /// <summary>
        /// The scan service scan stared.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void scanService_ScanStared(object sender, EventArgs e)
        {
            this.SelectedTab = 1;
        }
    }
}