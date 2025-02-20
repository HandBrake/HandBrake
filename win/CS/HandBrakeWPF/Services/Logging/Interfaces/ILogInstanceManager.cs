// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ILogInstanceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ILogInstanceManager type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Interfaces
{
    using System;
    using System.Collections.Generic;

    using HandBrakeWPF.Services.Logging.EventArgs;
    using HandBrakeWPF.Services.Queue;
    using HandBrakeWPF.Services.Queue.Interfaces;

    public interface ILogInstanceManager
    {
        event EventHandler<LogFileEventArgs> LogInstancesChanged;
        
        ILog ApplicationLogInstance { get; }

        /// <summary>
        /// Register an ILog instance.
        /// </summary>
        /// <param name="filename">
        /// This is the key associated with the log instance.
        /// </param>
        /// <param name="log">
        /// The ILog instance
        /// </param>
        /// <param name="isMaster">
        /// True indicates it's the log instance for the parent handbrake process.
        /// </param>
        void Register(string filename, ILog log, bool isMaster);

        /// <summary>
        /// Remove a log file when we are done with it.
        /// </summary>
        /// <param name="filename">The filename of the log to remove.</param>
        void Deregister(string filename);

        /// <summary>
        /// Reset the application log file.
        /// </summary>
        void ResetApplicationLog();

        /// <summary>
        /// Gets a list of files without their associated ILog instances.
        /// </summary>
        /// <returns>List of filenames being logged</returns>
        List<string> GetLogFiles();

        /// <summary>
        /// Get the ILog instance for a given filename key
        /// </summary>
        /// <param name="filename">The key of the log instance</param>
        /// <returns>An ILog instance or null if invalid key</returns>
        ILog GetLogInstance(string filename);

        /// <summary>
        /// Set the instance of the queue that is in-use. 
        /// </summary>
        void SetQueue(IQueueService queueService);
    }
}
