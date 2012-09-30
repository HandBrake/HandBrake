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
    using System.Collections.Generic;
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
        /// Backing field for the selected mode
        /// </summary>
        private int selectedMode;
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

            this.SelectedMode = this.encodeService.IsEncoding ? 0 : 1;
        }

        /// <summary>
        /// Gets Log.
        /// </summary>
        public string Log
        {
            get
            {
                return this.SelectedMode == 0 ? this.encodeService.ActivityLog : this.scanService.ActivityLog;
            }
        }

        /// <summary>
        /// Gets LogModes.
        /// </summary>
        public IEnumerable<string> LogModes
        {
            get
            {
                return new List<string> { "Encode Log", "Scan Log" };
            }
        }

        /// <summary>
        /// Gets or sets SelectedMode.
        /// </summary>
        public int SelectedMode
        {
            get
            {
                return selectedMode;
            }
            set
            {
                selectedMode = value;
                this.NotifyOfPropertyChange(() => this.SelectedMode);
                this.ChangeLogDisplay();
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
            Clipboard.SetDataObject(this.Log, true);
        }

        /// <summary>
        /// Handle the OnActivate Caliburn Event
        /// </summary>
        protected override void OnActivate()
        {
            this.scanService.ScanStared += ScanServiceScanStared;
            this.scanService.ScanCompleted += ScanServiceScanCompleted;
            this.encodeService.EncodeStarted += EncodeServiceEncodeStarted;
            this.encodeService.EncodeCompleted += EncodeServiceEncodeCompleted;
            this.encodeService.EncodeStatusChanged += this.EncodeServiceEncodeStatusChanged;
            this.scanService.ScanStatusChanged += this.ScanServiceScanStatusChanged;
            base.OnActivate();
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
            this.NotifyOfPropertyChange(() => this.Log);
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
            this.NotifyOfPropertyChange(() => this.Log);
        }

        /// <summary>
        /// Handle the OnDeactivate Caliburn Event
        /// </summary>
        /// <param name="close">
        /// The close.
        /// </param>
        protected override void OnDeactivate(bool close)
        {
            this.scanService.ScanStared -= ScanServiceScanStared;
            this.scanService.ScanCompleted -= ScanServiceScanCompleted;
            this.encodeService.EncodeStarted -= EncodeServiceEncodeStarted;
            this.encodeService.EncodeCompleted -= EncodeServiceEncodeCompleted;
            this.encodeService.EncodeStatusChanged -= this.EncodeServiceEncodeStatusChanged;
            this.scanService.ScanStatusChanged -= this.ScanServiceScanStatusChanged;

            base.OnDeactivate(close);
        }

        /// <summary>
        /// Change the Log Display
        /// </summary>
        private void ChangeLogDisplay()
        {
            this.NotifyOfPropertyChange(() => this.Log);
        }

        /// <summary>
        /// Encode Started Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeServiceEncodeStarted(object sender, EventArgs e)
        {
            this.SelectedMode = 0;
        }

        /// <summary>
        /// Scan Started Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ScanServiceScanStared(object sender, EventArgs e)
        {
            this.SelectedMode = 1;
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
            this.NotifyOfPropertyChange(() => this.Log);
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
            this.NotifyOfPropertyChange(() => this.Log);
        }
    }
}