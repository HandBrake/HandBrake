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
    using System.Linq;

    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;

    using Microsoft.Win32.SafeHandles;

    public class LogInstanceManager : ILogInstanceManager
    {
        private readonly IUserSettingService userSettingService;

        private readonly object instanceLock = new object();
        private Dictionary<string, ILog> logInstances = new Dictionary<string, ILog>();
        
        private int maxInstances;

        public LogInstanceManager(IUserSettingService userSettingService)
        {
            this.userSettingService = userSettingService;
            this.maxInstances = this.userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);
            userSettingService.SettingChanged += this.UserSettingService_SettingChanged;
        }

        private void UserSettingService_SettingChanged(object sender, HandBrakeWPF.EventArgs.SettingChangedEventArgs e)
        {
            if (e.Key == UserSettingConstants.SimultaneousEncodes)
            {
                this.maxInstances = this.userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);
            }
        }

        public event EventHandler NewLogInstanceRegistered;

        public string ApplicationAndScanLog { get; private set; }

        public ILog MasterLogInstance { get; private set; }

        public void RegisterLoggerInstance(string filename, ILog log, bool isMaster)
        {
            lock (this.instanceLock)
            {
                if (string.IsNullOrEmpty(this.ApplicationAndScanLog))
                {
                    // The application startup sets the initial log file.
                    this.ApplicationAndScanLog = filename;
                }

                this.logInstances.Add(filename, log);

                this.CleanupInstance();

                if (isMaster)
                {
                    this.MasterLogInstance = log;
                }
            }

            this.OnNewLogInstanceRegistered();
        }

        public List<string> GetLogFiles()
        {
            lock (this.instanceLock)
            {
                return this.logInstances.Keys.ToList();
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

        protected virtual void OnNewLogInstanceRegistered()
        {
            this.NewLogInstanceRegistered?.Invoke(this, System.EventArgs.Empty);
        }

        private void CleanupInstance()
        {
            List<string> encodeLogs = this.logInstances.Keys.Where(f => f.Contains(".encode.")).ToList();
            string removalKey = this.logInstances.Keys.OrderBy(k => k).FirstOrDefault(w => w.Contains(".encode."));
            if (encodeLogs.Count > this.maxInstances)
            {
                this.logInstances.Remove(removalKey);
            }
        }
    }
}
