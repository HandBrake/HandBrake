// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ILog.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ILog type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Interfaces
{
    using System;
    using System.Collections.Generic;

    using HandBrake.Worker.Logging.Models;

    using LogEventArgs = HandBrakeWPF.Services.Logging.EventArgs.LogEventArgs;

    /// <summary>
    /// The Log interface.
    /// </summary>
    public interface ILog
    {
        /// <summary>
        /// The message logged.
        /// </summary>
        event EventHandler<LogEventArgs> MessageLogged;

        /// <summary>
        /// The log reset event
        /// </summary>
        event EventHandler LogReset;

        /// <summary>
        /// Enable logging for this worker process.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <remarks>
        /// If this is not called, all log messages from libhb will be ignored.
        /// </remarks>
        void ConfigureLogging(string filename);

        /// <summary>
        /// Log a message.
        /// </summary>
        /// <param name="content">
        /// The content of the log message,
        /// </param>
        void LogMessage(string content);

        string GetFullLog();

        List<LogMessage> GetLogMessages();

        /// <summary>
        /// Empty the log cache and reset the log handler to defaults.
        /// </summary>
        void Reset();
    }
}