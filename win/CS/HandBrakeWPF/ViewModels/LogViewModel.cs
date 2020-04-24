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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Windows.Forms;

    using Caliburn.Micro;

    using HandBrake.Worker.Logging.Models;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Clipboard = System.Windows.Clipboard;
    using ILog = HandBrakeWPF.Services.Logging.Interfaces.ILog;
    using LogEventArgs = HandBrakeWPF.Services.Logging.EventArgs.LogEventArgs;
    using LogService = HandBrakeWPF.Services.Logging.LogService;

    public class LogViewModel : ViewModelBase, ILogViewModel
    {
        private readonly IErrorService errorService;

        private readonly ILogInstanceManager logInstanceManager;

        private ILog logService;
        private StringBuilder log = new StringBuilder();
        private long lastReadIndex;

        private string selectedLogFile;

        public LogViewModel(IErrorService errorService, ILogInstanceManager logInstanceManager)
        {
            this.errorService = errorService;
            this.logInstanceManager = logInstanceManager;
            this.Title = Resources.LogViewModel_Title;
            this.selectedLogFile = logInstanceManager.ApplicationAndScanLog;
        }

        public event EventHandler<LogEventArgs> LogMessageReceived;

        public string ActivityLog
        {
            get
            {
                return this.log.ToString();
            }
        }

        public BindingList<string> LogFiles
        {
            get
            {
                return new BindingList<string>(this.logInstanceManager.GetLogFiles());
            }
        }

        public string SelectedLogFile
        {
            get => this.selectedLogFile;
            set
            {
                if (value == this.selectedLogFile) return;
                this.selectedLogFile = value;
                this.NotifyOfPropertyChange(() => this.SelectedLogFile);

                this.ChangeLogFileView();
            }
        }

        public void OpenLogDirectory()
        {
            string logDir = DirectoryUtilities.GetLogDirectory();
            string windir = Environment.GetEnvironmentVariable("WINDIR");
            Process prc = new Process { StartInfo = { FileName = windir + @"\explorer.exe", Arguments = logDir } };
            prc.Start();
        }

        public void CopyLog()
        {
            try
            {
                Clipboard.SetDataObject(this.ActivityLog, true);
            }
            catch (Exception exc)
            {
                this.errorService.ShowError(Resources.Clipboard_Unavailable, Resources.Clipboard_Unavailable_Solution, exc);
            }
        }

        protected override void OnActivate()
        {
            this.logInstanceManager.NewLogInstanceRegistered += this.LogInstanceManager_NewLogInstanceRegistered;

            if (string.IsNullOrEmpty(this.SelectedLogFile))
            {
                base.OnActivate();
                return;
            }

            if (this.logService == null)
            {
                this.logService = this.logInstanceManager.GetLogInstance(this.SelectedLogFile);
            }

            this.NotifyOfPropertyChange(() => this.LogFiles);

            this.logService.MessageLogged += this.LogService_MessageLogged;
            this.logService.LogReset += LogService_LogReset;

            // Refresh the Log Display
            this.log.Clear();
            foreach (LogMessage logMessage in this.logService.GetLogMessages())
            {
                this.log.AppendLine(logMessage.Content);
                this.lastReadIndex = logMessage.MessageIndex;

                if (this.lastReadIndex > logMessage.MessageIndex)
                {
                    throw new Exception("Log Message Index Error");
                }
            }

            this.OnLogMessageReceived(null);
            this.NotifyOfPropertyChange(() => this.ActivityLog);

            base.OnActivate();
        }

        protected virtual void OnLogMessageReceived(LogEventArgs e)
        {
            var onLogMessageReceived = this.LogMessageReceived;
            if (onLogMessageReceived != null)
            {
                onLogMessageReceived.Invoke(this, e);
            }
        }

        protected override void OnDeactivate(bool close)
        {
            this.logService.MessageLogged -= this.LogService_MessageLogged;
            this.logService.LogReset -= this.LogService_LogReset;
            this.logInstanceManager.NewLogInstanceRegistered -= this.LogInstanceManager_NewLogInstanceRegistered;

            base.OnDeactivate(close);
        }

        private void ChangeLogFileView()
        {
            if (this.logService != null)
            {
                this.logService.MessageLogged -= this.LogService_MessageLogged;
                this.logService.LogReset -= this.LogService_LogReset;
            }

            this.logService = this.logInstanceManager.GetLogInstance(this.SelectedLogFile);

            if (this.logService != null)
            {
                OnActivate();
            }
        }

        private void LogService_LogReset(object sender, EventArgs e)
        {
            this.log.Clear();
            this.lastReadIndex = 0;

            foreach (LogMessage logMessage in this.logService.GetLogMessages())
            {
                this.log.AppendLine(logMessage.Content);
                this.lastReadIndex = logMessage.MessageIndex;

                if (this.lastReadIndex > logMessage.MessageIndex)
                {
                    throw new Exception("Log Message Index Error");
                }
            }

            this.NotifyOfPropertyChange(() => this.ActivityLog);
            this.OnLogMessageReceived(null);
        }

        private void LogService_MessageLogged(object sender, LogEventArgs e)
        {
            if (this.lastReadIndex < e.Log.MessageIndex)
            {
                Execute.OnUIThreadAsync(() =>
                        {
                            this.lastReadIndex = e.Log.MessageIndex;
                            this.log.AppendLine(e.Log.Content);
                            this.OnLogMessageReceived(e);
                            this.NotifyOfPropertyChange(() => this.ActivityLog);
                        });
            }
        }

        private void LogInstanceManager_NewLogInstanceRegistered(object sender, EventArgs e)
        {
            this.NotifyOfPropertyChange(() => this.LogFiles);
            this.SelectedLogFile = this.LogFiles.LastOrDefault();
        }
    }
}