﻿// --------------------------------------------------------------------------------------------------------------------
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

    using HandBrake.Worker.Logging.Models;

    using HandBrakeWPF.Utilities;

    using ILog = Interfaces.ILog;
    using LogEventArgs = EventArgs.LogEventArgs;

    public class LogService : ILog, IDisposable
    {
        private readonly object lockObject = new object();
        private readonly object fileWriterLock = new object();
        private readonly StringBuilder logBuilder = new StringBuilder();
        private readonly List<LogMessage> logMessages = new List<LogMessage>();

        private bool isLoggingEnabled;
        private int messageIndex;
        private string diskLogPath;
        private bool isDiskLoggingEnabled;
        private StreamWriter fileWriter;
        private string logHeader;

        public LogService()
        {
        }

        public event EventHandler<LogEventArgs> MessageLogged;

        public event EventHandler LogReset;

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

        public void LogMessage(string content)
        {
            if (!this.isLoggingEnabled)
            {
                return;
            }

            LogMessage msg = new LogMessage(content, this.messageIndex);
            lock (this.lockObject)
            {
                this.messageIndex = this.messageIndex + 1;
                this.logMessages.Add(msg);
                this.logBuilder.AppendLine(msg.Content);
                this.LogMessageToDisk(msg);

                if (this.logMessages.Count > 50000)
                {
                    this.messageIndex = this.messageIndex + 1;
                    msg = new LogMessage("Log Service Pausing. Too Many Log messages. This may indicate a problem with your encode.", this.messageIndex);
                    this.logMessages.Add(msg);
                    this.logBuilder.AppendLine(msg.Content);
                    this.LogMessageToDisk(msg);

                    this.isLoggingEnabled = false;
                }
            }

            this.OnMessageLogged(msg); // Must be outside lock to be thread safe. 
        }

        public void ConfigureLogging(string filename)
        {
            this.isLoggingEnabled = true;

            if (!string.IsNullOrEmpty(filename) && !Directory.Exists(Path.GetDirectoryName(filename)))
            {
                Directory.CreateDirectory(Path.GetDirectoryName(filename));
            }

            this.EnableLoggingToDisk(filename);

            this.logHeader = GeneralUtilities.CreateLogHeader().ToString();
            this.LogMessage(logHeader);
        }

        public string GetFullLog()
        {
            lock (this.lockObject)
            {
                return this.logBuilder.ToString();
            }
        }

        public List<LogMessage> GetLogMessages()
        {
            lock (this.lockObject)
            {
                return new List<LogMessage>(this.logMessages);
            }
        }

        public List<LogMessage> GetLogMessagesFromIndex(int index)
        {
            List<LogMessage> log = new List<LogMessage>();
            lock (this.lockObject)
            {
                // Note messageIndex is not 0 based.
                for (int i = index; i < this.messageIndex; i++)
                {
                    log.Add(this.logMessages[i]);
                }
            }

            return log;
        }

        public async void Reset()
        {
            lock (this.lockObject)
            {
                this.logMessages.Clear();
                this.logBuilder.Clear();
                this.messageIndex = 0;

                this.ShutdownFileWriter();

                if (this.fileWriter == null)
                {
                    this.isDiskLoggingEnabled = false;
                    this.EnableLoggingToDisk(this.diskLogPath);
                }

                if (!string.IsNullOrEmpty(this.logHeader))
                {
                    this.SetupLogHeader(this.logHeader);
                }

                this.OnLogReset();
            }
        }

        public void Dispose()
        {
            this.ShutdownFileWriter();
        }

        protected virtual void OnMessageLogged(LogMessage msg)
        {
            var onMessageLogged = this.MessageLogged;
            onMessageLogged?.Invoke(this, new LogEventArgs(msg));
        }

        private void ShutdownFileWriter()
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

        protected virtual void OnLogReset()
        {
            this.LogReset?.Invoke(this, System.EventArgs.Empty);
        }

        private void EnableLoggingToDisk(string logFile)
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

                if (File.Exists(logFile))
                {
                    File.Delete(logFile);
                }

                this.diskLogPath = logFile;
                this.isDiskLoggingEnabled = true;

                lock (this.fileWriterLock)
                {
                    this.fileWriter = new StreamWriter(logFile) { AutoFlush = true };
                }
            }
            catch (Exception exc)
            {
                this.LogMessage("Failed to Initialise Disk Logging. " + Environment.NewLine + exc);

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

        private void SetupLogHeader(string header)
        {
            this.logHeader = header;
            this.LogMessage(header);
        }

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
    }
}
