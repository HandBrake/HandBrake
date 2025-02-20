// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogInstanceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.EventArgs;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Queue.Interfaces;

    public class LogInstanceManager : ILogInstanceManager
    {
        private readonly IUserSettingService userSettingService;
        private IQueueService queueService;

        private readonly object instanceLock = new object();
        private Dictionary<string, ILog> logInstances = new Dictionary<string, ILog>();

        public event EventHandler<LogFileEventArgs> LogInstancesChanged;

        public LogInstanceManager(IUserSettingService userSettingService)
        {
            this.queueService = null;
            this.userSettingService = userSettingService;
        }

        public ILog ApplicationLogInstance { get; private set; }

        public void Register(string filename, ILog log, bool isMaster)
        {
            lock (this.instanceLock)
            {
                this.logInstances.Add(filename, log);

                if (isMaster)
                {
                    this.ApplicationLogInstance = log;
                }
            }

            this.OnLogInstancesChanged(new LogFileEventArgs(filename, log, true));
        }

        public void Deregister(string filename)
        {
            lock (this.instanceLock)
            {
                if (this.logInstances.ContainsKey(filename))
                {
                    ILog logInstance = this.logInstances[filename];
                    logInstance.Dispose();

                    this.logInstances.Remove(filename);
                }
            }

            this.OnLogInstancesChanged(new LogFileEventArgs(filename, null, false));
        }

        public void ResetApplicationLog()
        {
            if (this.CanResetLog())
            {
                this.ApplicationLogInstance.Reset();
                this.OnLogInstancesChanged(new LogFileEventArgs(this.ApplicationLogInstance.FileName, this.ApplicationLogInstance, true));
            }
        }

        public List<string> GetLogFiles()
        {
            lock (this.instanceLock)
            {
                List<string> encodeLogs = this.logInstances.Keys.Where(s => !s.Contains("main")).OrderBy(s => s).ToList();

                List<string> finalList = new List<string>();
                if (this.ApplicationLogInstance != null)
                {
                    finalList.Add(Path.GetFileName(this.ApplicationLogInstance.FileName));
                }

                finalList.AddRange(encodeLogs);

                return finalList;
            }
        }

        public ILog GetLogInstance(string filename)
        {
            lock (this.instanceLock)
            {
                if (string.IsNullOrEmpty(filename))
                {
                    return null;
                }

                ILog logger;
                if (this.logInstances.TryGetValue(filename, out logger))
                {
                    return logger;
                }
            }

            return null;
        }

        public void SetQueue(IQueueService queueService)
        {
            this.queueService = queueService;
        }

        protected virtual void OnLogInstancesChanged(LogFileEventArgs e)
        {
            this.LogInstancesChanged?.Invoke(this, e);
        }

        private bool CanResetLog()
        {
            // If we are running in-process and already encoding, don't allow log resets.
            if (!userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled))
            {
                if (this.queueService.IsEncoding || this.queueService.IsPaused || this.queueService.IsProcessing)
                {
                    return false;
                }
            }

            return true;
        }
    }
}
