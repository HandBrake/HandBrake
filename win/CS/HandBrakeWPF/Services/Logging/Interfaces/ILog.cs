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

    using LogEventArgs = HandBrakeWPF.Services.Logging.EventArgs.LogEventArgs;
    using LogLevel = HandBrakeWPF.Services.Logging.Model.LogLevel;
    using LogMessage = HandBrakeWPF.Services.Logging.Model.LogMessage;
    using LogMessageType = HandBrakeWPF.Services.Logging.Model.LogMessageType;

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
        /// Gets the log messages.
        /// </summary>
        IEnumerable<LogMessage> LogMessages { get; }

        /// <summary>
        /// Gets the activity log.
        /// </summary>
        string ActivityLog { get; }

        /// <summary>
        /// The reset.
        /// </summary>
        void Reset();

        /// <summary>
        /// The enable.
        /// </summary>
        void Enable();

        /// <summary>
        /// Log a message.
        /// </summary>
        /// <param name="content">
        /// The content of the log message,
        /// </param>
        /// <param name="type">
        /// The Message Type. (i.e. where it came from)
        /// </param>
        /// <param name="level">
        /// The log level
        /// </param>
        void LogMessage(string content, LogMessageType type, LogLevel level);

        /// <summary>
        /// Enable Logging to Disk
        /// </summary>
        /// <param name="logFile">
        /// The log file to write to.
        /// </param>
        /// <param name="deleteCurrentLogFirst">
        /// Delete the current log file if it exists.
        /// </param>
        void EnableLoggingToDisk(string logFile, bool deleteCurrentLogFirst);

        /// <summary>
        /// The setup log header.
        /// </summary>
        /// <param name="header">
        /// The header.
        /// </param>
        void SetupLogHeader(string header);
    }
}