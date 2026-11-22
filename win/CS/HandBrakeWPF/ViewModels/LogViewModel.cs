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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Numerics;
    using System.Text;
    using System.Windows.Threading;

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

        // The log StringBuilder, lastReadIndex, logService and LogFiles are only ever touched on the UI thread.
        private ILog logService;
        private StringBuilder log = new StringBuilder();
        private long lastReadIndex = -1;
        private LogFile selectedLogFile;
        private bool isActive;

        public LogViewModel(IErrorService errorService, ILogInstanceManager logInstanceManager, IQueueService queueService)
        {
            this.errorService = errorService;
            this.logInstanceManager = logInstanceManager;
            this.queueService = queueService;
            this.Title = Resources.LogViewModel_Title;
        }

        public event EventHandler<LogEventArgs> LogMessageReceived;

        public event EventHandler LogResetEvent;

        private string searchText = string.Empty;

        public string ActivityLog => this.log.ToString();

        public string SearchText
        {
            get => this.searchText;
            set
            {
                if (value == this.searchText)
                {
                    return;
                }

                this.searchText = value;
                this.NotifyOfPropertyChange(() => this.SearchText);
            }
        }

        public bool MatchesFilter(string line)
        {
            string filter = this.SearchText;
            return string.IsNullOrEmpty(filter) || (line != null && line.Contains(filter, StringComparison.OrdinalIgnoreCase));
        }

        /// <summary>
        /// Applies the current search filter, line by line, to a single log message.
        /// </summary>
        /// <returns>The text to display, or null if nothing in the message matches the filter.</returns>
        public string FilterMessage(string content)
        {
            if (string.IsNullOrEmpty(this.SearchText))
            {
                return content ?? string.Empty;
            }

            if (content == null)
            {
                return null;
            }

            var lines = content.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None).Where(this.MatchesFilter).ToList();
            return lines.Count == 0 ? null : string.Join(Environment.NewLine, lines);
        }

        public string GetFilteredLog()
        {
            string full = this.ActivityLog.TrimStart();
            if (string.IsNullOrEmpty(this.SearchText))
            {
                return full;
            }

            var lines = full.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None).Where(this.MatchesFilter).ToList();
            return lines.Count == 0 ? string.Empty : string.Join(Environment.NewLine, lines) + Environment.NewLine;
        }

        public BindingList<LogFile> LogFiles { get; private set; }

        public long LogFileCount => this.GetLogFileCount();

        public decimal LogFileSizeTotal => this.GetLogFileSizeTotal();

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
            using Process prc = Process.Start(new ProcessStartInfo("explorer.exe", $"\"{logDir}\"") { UseShellExecute = true });
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

        public void ClearCompleted()
        {
            if (this.LogFiles == null)
            {
                return;
            }

            List<LogFile> localCopy = this.LogFiles.ToList();

            foreach (var checkLog in localCopy)
            {
                if (checkLog.IsComplete)
                {
                    this.LogFiles.Remove(checkLog);
                }
            }

            if (!this.LogFiles.Any())
            {
                this.DetachLogService();
                this.selectedLogFile = null;
                this.NotifyOfPropertyChange(() => this.SelectedLogFile);
                this.log.Clear();
                this.OnLogResetEvent();
            }
            else if (this.SelectedLogFile == null || !this.LogFiles.Contains(this.SelectedLogFile))
            {
                this.selectedLogFile = null; // The old selection was removed, force a reload.
                this.SelectedLogFile = this.LogFiles.LastOrDefault();
            }
        }

        public override void Activate()
        {
            this.isActive = true;

            this.logInstanceManager.LogInstancesChanged -= this.LogInstanceManager_NewLogInstanceRegistered;
            this.logInstanceManager.LogInstancesChanged += this.LogInstanceManager_NewLogInstanceRegistered;

            this.CollectLogFiles(null);

            base.Activate();
        }

        public override void Deactivate()
        {
            this.isActive = false;

            this.logInstanceManager.LogInstancesChanged -= this.LogInstanceManager_NewLogInstanceRegistered;
            this.DetachLogService();
            this.SelectedLogFile = null;

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

        private const string MainLogName = "activity_log_main";

        private static void RunOnUiThread(Action action)
        {
            // Always queue (never run inline) so that ordering is preserved between events raised on any thread.
            // Asynchronous so that a log thread can never block on, or deadlock with, the UI thread.
            Dispatcher dispatcher = System.Windows.Application.Current?.Dispatcher;
            if (dispatcher == null)
            {
                action();
                return;
            }

            dispatcher.BeginInvoke(DispatcherPriority.Background, action);
        }

        private void DetachLogService()
        {
            if (this.logService != null)
            {
                this.logService.MessageLogged -= this.LogService_MessageLogged;
                this.logService.LogReset -= this.LogService_LogReset;
                this.logService = null;
            }
        }

        private void ChangeLogFileView()
        {
            this.DetachLogService();

            LogFile selected = this.SelectedLogFile;
            if (selected == null)
            {
                return;
            }

            this.lastReadIndex = -1;

            ILog service = this.logInstanceManager.GetLogInstance(selected.LogFileName);
            if (service == null)
            {
                ILog appLog = this.logInstanceManager.ApplicationLogInstance;
                if (appLog?.FileName != null && selected.LogFileName.Equals(Path.GetFileName(appLog.FileName), StringComparison.InvariantCultureIgnoreCase))
                {
                    service = appLog;
                }
            }

            // This is not an active log, so read from disk.
            if (service == null)
            {
                this.LoadLogFromDisk(selected.LogFileName);
                return;
            }

            // Active in-progress log, read from the log service.
            this.logService = service;
            service.MessageLogged += this.LogService_MessageLogged;
            service.LogReset += this.LogService_LogReset;

            this.RefreshFromLogService(service);
        }

        private void LoadLogFromDisk(string logFileName)
        {
            this.log.Clear();

            try
            {
                string logFile = Path.Combine(DirectoryUtilities.GetLogDirectory(), logFileName);
                if (File.Exists(logFile))
                {
                    using (var fs = new FileStream(logFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
                    {
                        using (StreamReader logReader = new StreamReader(fs))
                        {
                            this.log.Append(logReader.ReadToEnd());
                        }
                    }
                }
                else
                {
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

        private void RefreshFromLogService(ILog service)
        {
            this.log.Clear();
            this.lastReadIndex = -1;

            foreach (LogMessage logMessage in service.GetLogMessages())
            {
                this.log.AppendLine(logMessage.Content);
                this.lastReadIndex = logMessage.MessageIndex;
            }

            this.OnLogResetEvent();
        }

        private void LogService_LogReset(object sender, EventArgs e)
        {
            // Can be raised from any thread.
            ILog source = sender as ILog;
            RunOnUiThread(() =>
            {
                if (source != null && ReferenceEquals(source, this.logService))
                {
                    this.RefreshFromLogService(source);
                }
            });
        }

        private void LogService_MessageLogged(object sender, LogEventArgs e)
        {
            // Can be raised from any thread. Validate on the UI thread so that stale or duplicate messages
            // (e.g. already included in the snapshot, or from a log we have since switched away from) are ignored.
            ILog source = sender as ILog;
            RunOnUiThread(() =>
            {
                if (source == null || !ReferenceEquals(source, this.logService) || e.Log.MessageIndex <= this.lastReadIndex)
                {
                    return;
                }

                this.lastReadIndex = e.Log.MessageIndex;
                this.log.AppendLine(e.Log.Content);
                this.OnLogMessageReceived(e);
            });
        }

        private void LogInstanceManager_NewLogInstanceRegistered(object sender, LogFileEventArgs e)
        {
            // Can be raised from any thread. Only follow newly registered logs; ignore de-registrations.
            string filename = e.IsNew ? e.FileName : null;
            RunOnUiThread(() =>
            {
                if (this.isActive)
                {
                    this.CollectLogFiles(filename);
                }
            });
        }

        private void CollectLogFiles(string filename)
        {
            LogFile previous = this.selectedLogFile;

            this.SelectedLogFile = null; // Fix for the ComboBox resetting this later as we have to rebuild the collection here.

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

            LogFile FindByName(string name) => this.LogFiles.FirstOrDefault(c => c.LogFileName.Equals(name, StringComparison.InvariantCultureIgnoreCase));

            // Only jump to a newly registered log if the user isn't actively watching another running encode.
            LogFile target = null;
            if (!string.IsNullOrEmpty(filename) && (previous == null || previous.IsComplete || previous.LogFileName.Contains(MainLogName)))
            {
                target = FindByName(filename);
            }

            if (target == null && previous != null)
            {
                target = FindByName(previous.LogFileName);
            }

            target ??= this.LogFiles.LastOrDefault(c => !c.LogFileName.Contains(MainLogName));
            target ??= this.LogFiles.LastOrDefault();

            if (target != null && target.Equals(previous))
            {
                // Same log as before, so the view is still attached to it. Don't reload it.
                this.selectedLogFile = target;
                this.NotifyOfPropertyChange(() => this.SelectedLogFile);
            }
            else
            {
                this.SelectedLogFile = target;
            }
        }

        private long GetLogFileCount()
        {
            try
            {
                string logDir = DirectoryUtilities.GetLogDirectory();
                return Directory.Exists(logDir) ? Directory.GetFiles(logDir, "*.txt").LongLength : 0;
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
                return 0;
            }
        }

        private decimal GetLogFileSizeTotal()
        {
            long totalBytes = 0;

            try
            {
                string logDir = DirectoryUtilities.GetLogDirectory();
                if (!Directory.Exists(logDir))
                {
                    return 0;
                }

                foreach (string logPath in Directory.GetFiles(logDir))
                {
                    try
                    {
                        totalBytes += new FileInfo(logPath).Length;
                    }
                    catch (IOException exc)
                    {
                        Debug.WriteLine(exc); // File removed while enumerating.
                    }
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }

            return Math.Round(totalBytes / 1024m / 1024m, 2);
        }
    }
}
