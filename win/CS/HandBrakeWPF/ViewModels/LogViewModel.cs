﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.Text;
    using System.Windows;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Services.Logging;
    using HandBrake.ApplicationServices.Services.Logging.EventArgs;
    using HandBrake.ApplicationServices.Services.Logging.Model;

    using HandBrakeWPF.ViewModels.Interfaces;

    using ILog = HandBrake.ApplicationServices.Services.Logging.Interfaces.ILog;

    /// <summary>
    /// The Log View Model
    /// </summary>
    public class LogViewModel : ViewModelBase, ILogViewModel
    {
        #region Private Fields

        private readonly ILog logService;
        private StringBuilder log = new StringBuilder();
        private long lastReadIndex;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LogViewModel"/> class.
        /// </summary>
        public LogViewModel()
        {
            this.logService = LogService.GetLogger();
            this.Title = "Log Viewer";
        }

        /// <summary>
        /// Gets Log.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                return this.log.ToString();
            }
        }

        /// <summary>
        /// The log message received.
        /// </summary>
        public event EventHandler<LogEventArgs> LogMessageReceived;
    
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
            Clipboard.SetDataObject(this.ActivityLog, true);
        }

        /// <summary>
        /// Handle the OnActivate Caliburn Event
        /// </summary>
        protected override void OnActivate()
        {
            this.logService.MessageLogged += this.LogService_MessageLogged;
            this.logService.LogReset += LogService_LogReset;

            // Refresh the Log Display
            this.log.Clear();
            foreach (LogMessage logMessage in this.logService.LogMessages)
            {
                this.log.AppendLine(logMessage.Content);
                this.lastReadIndex = logMessage.MessageIndex;

                if (this.lastReadIndex > logMessage.MessageIndex)
                {
                    throw new Exception("Log Message Index Error");
                }
            }

            this.OnLogMessageReceived(null);
            this.NotifyOfPropertyChange("ActivityLog");

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
            this.logService.MessageLogged -= this.LogService_MessageLogged;
            this.logService.LogReset -= this.LogService_LogReset;

            base.OnDeactivate(close);
        }

        /// <summary>
        /// The log service_ log reset.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void LogService_LogReset(object sender, EventArgs e)
        {
            this.log.Clear();
            this.lastReadIndex = 0;

            foreach (LogMessage logMessage in this.logService.LogMessages)
            {
                this.log.AppendLine(logMessage.Content);
                this.lastReadIndex = logMessage.MessageIndex;

                if (this.lastReadIndex > logMessage.MessageIndex)
                {
                    throw new Exception("Log Message Index Error");
                }
            }

            this.NotifyOfPropertyChange("ActivityLog");
            this.OnLogMessageReceived(null);
        }

        /// <summary>
        /// The log service_ message logged.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void LogService_MessageLogged(object sender, LogEventArgs e)
        {
            if (this.lastReadIndex < e.Log.MessageIndex)
            {
                Execute.OnUIThreadAsync(
                    () =>
                        {
                            this.lastReadIndex = e.Log.MessageIndex;
                            this.log.AppendLine(e.Log.Content);
                            this.OnLogMessageReceived(e);
                            this.NotifyOfPropertyChange("ActivityLog");
                        });
            }
        }

        /// <summary>
        /// Trigger a faster / smoother way of updating the log window.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected virtual void OnLogMessageReceived(LogEventArgs e)
        {
            var onLogMessageReceived = this.LogMessageReceived;
            if (onLogMessageReceived != null)
            {
                onLogMessageReceived.Invoke(this, e);
            }
        }
    }
}