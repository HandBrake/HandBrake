// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The log service.
//   For now, this is just a simple logging service but we could provide support for a formal logging library later.
//   Also, we can consider providing the UI layer with more functional logging. (i.e levels, time/date, highlighting etc)
//   The Interop Classes are not very OO friendly, so this is going to be a static class.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.EventArgs;

    using ILog = Interfaces.ILog;
    using LogEventArgs = EventArgs.LogEventArgs;
    using LogLevel = Model.LogLevel;
    using LogMessage = Model.LogMessage;
    using LogMessageType = Model.LogMessageType;

    /// <summary>
    /// The log helper.
    /// </summary>
    public class LogService : ILog
    {
        // TODO List.
        // Maybe make the event weak?
        // Make this class Thread Safe.
        private static ILog loggerInstance;
        private readonly object lockObject = new object();
        private readonly object fileWriterLock = new object();
        private readonly StringBuilder logBuilder = new StringBuilder();
 
        private LogLevel currentLogLevel = LogLevel.Error;
        private bool isLoggingEnabled;
        private List<LogMessage> logMessages = new List<LogMessage>(); 
        private long messageIndex;
        private string diskLogPath;
        private bool deleteLogFirst;
        private bool isDiskLoggingEnabled;
        private StreamWriter fileWriter;
        private string logHeader;

        public LogService()
        {
            HandBrakeUtils.MessageLogged += this.HandBrakeUtils_MessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeUtils_ErrorLogged;
        }

        /// <summary>
        /// Fires when a new QueueTask starts
        /// </summary>
        public event EventHandler<LogEventArgs> MessageLogged;

        /// <summary>
        /// The log reset event
        /// </summary>
        public event EventHandler LogReset;

        /// <summary>
        /// Gets the log messages.
        /// </summary>
        public IEnumerable<LogMessage> LogMessages
        {
            get
            {
                lock (this.lockObject)
                {
                    return this.logMessages.ToList();
                }
            }
        }

        /// <summary>
        /// Gets the Activity Log as a string.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                lock (this.lockObject)
                {
                    return this.logBuilder.ToString();
                }
            }
        }

        /// <summary>
        /// Log message.
        /// </summary>
        /// <param name="content">
        /// The content.
        /// </param>
        /// <param name="type">
        /// The type.
        /// </param>
        /// <param name="level">
        /// The level.
        /// </param>
        public void LogMessage(string content, LogMessageType type, LogLevel level)
        {
            if (!this.isLoggingEnabled)
            {
                return;
            }

            if (level > this.currentLogLevel)
            {
                return;
            }

            LogMessage msg = new LogMessage(content, type, level, this.messageIndex);
            lock (this.lockObject)
            {
                this.messageIndex = this.messageIndex + 1;   
                this.logMessages.Add(msg);
                this.logBuilder.AppendLine(msg.Content);
                this.LogMessageToDisk(msg);

                if (this.logMessages.Count > 50000)
                {
                    this.messageIndex = this.messageIndex + 1;
                    msg = new LogMessage(
                            "Log Service Pausing. Too Many Log messages. This may indicate a problem with your encode.",
                            LogMessageType.Application,
                            LogLevel.Error,
                            this.messageIndex);
                    this.logMessages.Add(msg);
                    this.logBuilder.AppendLine(msg.Content);
                    this.LogMessageToDisk(msg);

                    this.Disable();
                }
            }

            this.OnMessageLogged(msg); // Must be outside lock to be thread safe. 
        }

        /// <summary>
        /// Gets an shared instance of the logger. Logging is enabled by default
        /// You can turn it off by calling Disable() if you don't want it.
        /// </summary>
        /// <returns>
        /// An instance of this logger.
        /// </returns>
        public static ILog GetLogger()
        {
            return loggerInstance ?? (loggerInstance = new LogService());
        }

        /// <summary>
        /// The set log level. Default: Info.
        /// </summary>
        /// <param name="level">
        /// The level.
        /// </param>
        public void SetLogLevel(LogLevel level)
        {
            this.currentLogLevel = level;
        }

        /// <summary>
        /// The enable.
        /// </summary>
        public void Enable()
        {
            this.isLoggingEnabled = true;
        }

        /// <summary>
        /// Enable Logging to Disk
        /// </summary>
        /// <param name="logFile">
        /// The log file to write to.
        /// </param>
        /// <param name="deleteCurrentLogFirst">
        /// Delete the current log file if it exists.
        /// </param>
        public void EnableLoggingToDisk(string logFile, bool deleteCurrentLogFirst)
        {
            if (this.isDiskLoggingEnabled)
            {
                throw new Exception("Disk Logging already enabled!");
            }

            try
            {
                if (!Directory.Exists(Path.GetDirectoryName(logFile)))
                {
                    throw new Exception("Log Directory does not exist. This service will not create it for you!");
                }

                if (deleteCurrentLogFirst && File.Exists(logFile))
                {
                    File.Delete(logFile);
                }

                this.diskLogPath = logFile;
                this.isDiskLoggingEnabled = true;
                this.deleteLogFirst = deleteCurrentLogFirst;

                lock (this.fileWriterLock)
                {
                    this.fileWriter = new StreamWriter(logFile) { AutoFlush = true };
                }
            }
            catch (Exception exc)
            {
                this.LogMessage("Failed to Initialise Disk Logging. " + Environment.NewLine + exc, LogMessageType.Application, LogLevel.Error);

                if (this.fileWriter != null)
                {
                    lock (this.fileWriterLock)
                    {
                        this.fileWriter.Flush();
                        this.fileWriter.Close();
                        this.fileWriter.Dispose();
                    }
                }
            }
        }

        /// <summary>
        /// The setup log header.
        /// </summary>
        /// <param name="header">
        /// The header.
        /// </param>
        public void SetupLogHeader(string header)
        {
            this.logHeader = header;
            this.LogMessage(header, LogMessageType.Application, LogLevel.Info);
        }

        /// <summary>
        /// The disable.
        /// </summary>
        public void Disable()
        {
            this.isLoggingEnabled = false;
        }

        /// <summary>
        /// Clear the log messages collection.
        /// </summary>
        public void Reset()
        {
            lock (this.lockObject)
            {
                this.logMessages.Clear();
                this.logBuilder.Clear();
                this.messageIndex = 0;
               
                try
                {
                    lock (this.fileWriterLock)
                    {
                        if (this.fileWriter != null)
                        {
                            this.fileWriter.Flush();
                            this.fileWriter.Close();
                            this.fileWriter.Dispose();
                        }

                        this.fileWriter = null;
                    }
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                }

                if (this.fileWriter == null)
                {
                    this.isDiskLoggingEnabled = false;
                    this.EnableLoggingToDisk(this.diskLogPath, this.deleteLogFirst);
                }

                if (!string.IsNullOrEmpty(this.logHeader))
                {
                    this.SetupLogHeader(this.logHeader);
                }

                this.OnLogReset();
            }
        }

        /// <summary>
        /// Called when a log message is created.
        /// </summary>
        /// <param name="msg">
        /// The Log Message
        /// </param>
        protected virtual void OnMessageLogged(LogMessage msg)
        {
            var onMessageLogged = this.MessageLogged;
            if (onMessageLogged != null)
            {
                onMessageLogged.Invoke(this, new LogEventArgs(msg));
            }
        }

        /// <summary>
        /// Shutdown and Dispose of the File Writer.
        /// </summary>
        protected void ShutdownFileWriter()
        {
            try
            {
                lock (this.fileWriterLock)
                {
                    if (this.fileWriter != null)
                    {
                        this.fileWriter.Flush();
                        this.fileWriter.Close();
                        this.fileWriter.Dispose();
                    }

                    this.fileWriter = null;
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc); // This exception doesn't warrant user interaction, but it should be logged
            }
        }

        /// <summary>
        /// Trigger the Event to notify any subscribers that the log has been reset.
        /// </summary>
        protected virtual void OnLogReset()
        {
            this.LogReset?.Invoke(this, System.EventArgs.Empty);
        }

        /// <summary>
        /// Helper method for logging content to disk
        /// </summary>
        /// <param name="msg">
        /// Log message to write.
        /// </param>
        private void LogMessageToDisk(LogMessage msg)
        {
            if (!this.isDiskLoggingEnabled)
            {
                return;
            }

            try
            {
                lock (this.fileWriterLock)
                {
                    if (this.fileWriter != null && this.fileWriter.BaseStream.CanWrite)
                    {
                        this.fileWriter.WriteLine(msg.Content);
                    }
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc); // This exception doesn't warrant user interaction, but it should be logged
            }
        }

        private void HandBrakeUtils_ErrorLogged(object sender, MessageLoggedEventArgs e)
        {
            if (e == null || string.IsNullOrEmpty(e.Message))
            {
                return;
            }

            this.LogMessage(e.Message, LogMessageType.ScanOrEncode, LogLevel.Error);
        }

        private void HandBrakeUtils_MessageLogged(object sender, MessageLoggedEventArgs e)
        {
            if (e == null || string.IsNullOrEmpty(e.Message))
            {
                return;
            }

            this.LogMessage(e.Message, LogMessageType.ScanOrEncode, LogLevel.Info);
        }
    }
}
