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

    using HandBrakeWPF.Services.Logging.Interfaces;

    public class LogInstanceManager : ILogInstanceManager
    {
        private Dictionary<string, ILog> logInstances = new Dictionary<string, ILog>();

        public LogInstanceManager()
        {
        }

        public event EventHandler NewLogInstanceRegistered;

        public string ApplicationAndScanLog { get; private set; }

        public void RegisterLoggerInstance(string filename, ILog log)
        {
            if (string.IsNullOrEmpty(this.ApplicationAndScanLog))
            {
                // The application startup sets the initial log file.
                this.ApplicationAndScanLog = filename;
            }

            this.logInstances.Add(filename, log);
            this.OnNewLogInstanceRegistered();
        }

        public List<string> GetLogFiles()
        {
            return this.logInstances.Keys.ToList();
        }

        public ILog GetLogInstance(string filename)
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

            return null;
        }

        protected virtual void OnNewLogInstanceRegistered()
        {
            this.NewLogInstanceRegistered?.Invoke(this, System.EventArgs.Empty);
        }
    }
}
