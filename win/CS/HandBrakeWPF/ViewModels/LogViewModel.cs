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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model.Logging;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.EventArgs;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Logging.Model;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Clipboard = System.Windows.Clipboard;
    using ILog = Services.Logging.Interfaces.ILog;
    using LogEventArgs = Services.Logging.EventArgs.LogEventArgs;

    public class LogViewModel : ViewModelBase, ILogViewModel
    {
        private readonly IErrorService errorService;
        private readonly ILogInstanceManager logInstanceManager;
        private readonly IQueueService queueService;
        private readonly object readLockObject = new object();

        private ILog logService;
        private StringBuilder log = new StringBuilder();
        private long lastReadIndex;
        private LogFile selectedLogFile;

        public LogViewModel(IErrorService errorService, ILogInstanceManager logInstanceManager, IQueueService queueService)
        {
            this.errorService = errorService;
            this.logInstanceManager = logInstanceManager;
            this.queueService = queueService;
            this.Title = Resources.LogViewModel_Title;
        }

        public event EventHandler<LogEventArgs> LogMessageReceived;

        public event EventHandler LogResetEvent;

        public string ActivityLog => this.log.ToString();

        public BindingList<LogFile> LogFiles { get; private set; }

        public LogFile SelectedLogFile
        {
            get => this.selectedLogFile;
            set
            {
                if (Equals(value, this.selectedLogFile))
                {
                    return;
                }
                
                this.selectedLogFile = value;
                this.NotifyOfPropertyChange(() => this.SelectedLogFile);

                if (value != null)
                {
                    this.ChangeLogFileView();
                }
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

        public override void Activate()
        {
            this.logInstanceManager.LogInstancesChanged += this.LogInstanceManager_NewLogInstanceRegistered;

            this.CollectLogFiles(null);

            base.Activate();
        }

        public override void Deactivate()
        {
            if (this.logService != null)
            {
                this.logService.MessageLogged -= this.LogService_MessageLogged;
                this.logService.LogReset -= this.LogService_LogReset;
            }

            this.SelectedLogFile = null;
            this.logInstanceManager.LogInstancesChanged -= this.LogInstanceManager_NewLogInstanceRegistered;

            base.Deactivate();
        }

        protected virtual void OnLogResetEvent()
        {
            this.LogResetEvent?.Invoke(this, EventArgs.Empty);
        }

        protected virtual void OnLogMessageReceived(LogEventArgs e)
        {
            var onLogMessageReceived = this.LogMessageReceived;
            onLogMessageReceived?.Invoke(this, e);
        }

        private void ChangeLogFileView()
        {
            if (this.logService != null)
            {
                this.logService.MessageLogged -= this.LogService_MessageLogged;
                this.logService.LogReset -= this.LogService_LogReset;
            }

            if (this.SelectedLogFile == null)
            {
                return;
            }

            this.logService = this.logInstanceManager.GetLogInstance(this.SelectedLogFile.LogFileName);
            if (this.logService == null && this.SelectedLogFile != null && this.SelectedLogFile.LogFileName.Equals(this.logInstanceManager.ApplicationLogInstance.FileName, StringComparison.InvariantCultureIgnoreCase))
            {
                this.logService = this.logInstanceManager.ApplicationLogInstance;
            }
            
            string logDir = DirectoryUtilities.GetLogDirectory();
            string logFile = Path.Combine(logDir, this.selectedLogFile.LogFileName);

            // This is not an active log, so read from disk.
            if (this.logService == null)
            {
                try
                {
                    if (File.Exists(logFile))
                    {
                        this.log.Clear();
                        using (var fs = new FileStream(logFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
                        {
                            using (StreamReader logReader = new StreamReader(fs))
                            {
                                string logContent = logReader.ReadToEnd();
                                this.log.AppendLine(logContent);
                            }
                        }
                    }
                    else
                    {
                        this.log.Clear();
                        this.log.AppendLine("Sorry, The log file was not found.");
                    }
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                    this.log.AppendLine(exc.ToString());
                }

                this.OnLogResetEvent();
            }

            // Active in-progress log, read from the log service.
            if (this.logService != null)
            {
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

                this.OnLogResetEvent();
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

            this.OnLogResetEvent();
        }

        private void LogService_MessageLogged(object sender, LogEventArgs e)
        {
            if (this.lastReadIndex < e.Log.MessageIndex)
            {
                ThreadHelper.OnUIThread(() =>
                        {
                            this.lastReadIndex = e.Log.MessageIndex;
                            this.log.AppendLine(e.Log.Content);
                            this.OnLogMessageReceived(e);
                        });
            }
        }

        private void LogInstanceManager_NewLogInstanceRegistered(object sender, LogFileEventArgs e)
        {
            this.CollectLogFiles(e.FileName);
        }
        
        private void CollectLogFiles(string filename)
        {
            lock (readLockObject)
            {
                BindingList<string> activeLogs = new BindingList<string>(this.logInstanceManager.GetLogFiles());
                BindingList<LogFile> logfiles = new BindingList<LogFile>();

                // Add Inactive Logs First.
                foreach (string logFile in this.queueService.GetLogFilePaths())
                {
                    logfiles.Add(new LogFile(Path.GetFileName(logFile), true));
                }

                // Add active logs second.
                foreach (var log in activeLogs)
                {
                    logfiles.Add(new LogFile(log, false));
                }

                this.LogFiles = logfiles;
                this.NotifyOfPropertyChange(() => this.LogFiles);


                if (!string.IsNullOrEmpty(filename))
                {
                    this.SelectedLogFile = this.LogFiles.FirstOrDefault(c => c.LogFileName.Equals(filename, StringComparison.InvariantCultureIgnoreCase));
                }
                else
                {
                    this.SelectedLogFile = this.LogFiles.LastOrDefault(c => !c.LogFileName.Contains("activity_log_main"));
                }

                this.SelectedLogFile ??= this.LogFiles.LastOrDefault();
            }
        }
    }
}