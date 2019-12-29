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

    using HandBrake.Worker.Logging.Interfaces;
    using HandBrake.Worker.Logging.Models;

    using HandBrakeWPF.Services.Logging.Model;

    using LogEventArgs = HandBrakeWPF.Services.Logging.EventArgs.LogEventArgs;

    /// <summary>
    /// The Log interface.
    /// </summary>
    public interface ILog : ILogHandler
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
        /// <param name="config">
        /// Configuration for the logger.
        /// </param>
        /// <remarks>
        /// If this is not called, all log messages from libhb will be ignored.
        /// </remarks>
        void ConfigureLogging(LogHandlerConfig config);

        /// <summary>
        /// Log a message.
        /// </summary>
        /// <param name="content">
        /// The content of the log message,
        /// </param>
        void LogMessage(string content);
    }
}