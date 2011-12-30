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

    using Caliburn.Micro;

    using Interfaces;

    using HandBrake.ApplicationServices.EventArgs;

    /// <summary>
    /// The Log View Model
    /// </summary>
    public class LogViewModel : ViewModelBase, ILogViewModel
    {
        /**
         * TODO
         * - Live update the log file.
         */

        #region Private Fields

        /// <summary>
        /// Backing field for the encodeService service
        /// </summary>
        private readonly IEncode encodeService;

        /// <summary>
        /// Backing field for the Scan Service
        /// </summary>
        private readonly IScan scanService;

        /// <summary>
        /// Backing field for the selected mode
        /// </summary>
        private int selectedMode;

        /// <summary>
        /// Backing field for the log info.
        /// </summary>
        private string log;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LogViewModel"/> class.
        /// </summary>
        /// <param name="windowManager">
        /// The window manager.
        /// </param>
        /// <param name="encodeService">
        /// The encode service.
        /// </param>
        /// <param name="scanService">
        /// The scan service.
        /// </param>
        public LogViewModel(IWindowManager windowManager, IEncode encodeService, IScan scanService)
        {
            this.encodeService = encodeService;
            this.scanService = scanService;
            this.Title = "Log Viewer";
            this.SelectedMode = 0;
        }

        /// <summary>
        /// Gets or sets Log.
        /// </summary>
        public string Log
        {
            get
            {
                return log;
            }
            set
            {
                log = value;
                this.NotifyOfPropertyChange("Log");
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
                this.NotifyOfPropertyChange("SelectedMode");
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
            this.scanService.ScanStared += scanService_ScanStared;
            this.scanService.ScanCompleted += scanService_ScanCompleted;
            this.encodeService.EncodeStarted += encodeService_EncodeStarted;
            this.encodeService.EncodeCompleted += encodeService_EncodeCompleted;
            base.OnActivate();
        }

        /// <summary>
        /// Handle the OnDeactivate Caliburn Event
        /// </summary>
        /// <param name="close">
        /// The close.
        /// </param>
        protected override void OnDeactivate(bool close)
        {
            this.scanService.ScanStared -= scanService_ScanStared;
            this.encodeService.EncodeStarted -= encodeService_EncodeStarted;
            this.Load();
            base.OnDeactivate(close);
        }

        /// <summary>
        /// Change the Log Display
        /// </summary>
        private void ChangeLogDisplay()
        {
            this.Log = this.SelectedMode == 0 ? this.encodeService.ActivityLog : this.scanService.ActivityLog;
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
        private void encodeService_EncodeStarted(object sender, EventArgs e)
        {
            this.SelectedMode = 0;
            this.Log = this.encodeService.ActivityLog;
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
        private void scanService_ScanStared(object sender, EventArgs e)
        {
            this.SelectedMode = 1;
            this.Log = this.scanService.ActivityLog;
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
        private void scanService_ScanCompleted(object sender, ScanCompletedEventArgs e)
        {
            if (this.SelectedMode == 1)
            {
                this.Log = this.scanService.ActivityLog;
            }
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
        private void encodeService_EncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            if (this.SelectedMode == 0)
            {
                this.Log = this.encodeService.ActivityLog;
            }
        }
    }
}