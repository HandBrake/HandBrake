// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogHandler.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The log service.
//   The Interop Classes are not very OO friendly. This is a shim over the logging code to allow a nice Web API to be built on top.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Logging
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;

    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Worker.Logging.Interfaces;
    using HandBrake.Worker.Logging.Models;

    public class LogHandler : ILogHandler
    {
        private readonly object lockObject = new object();
        private readonly object fileWriterLock = new object();
        private readonly StringBuilder logBuilder = new StringBuilder();
        private readonly List<LogMessage> logMessages = new List<LogMessage>();

        private bool isLoggingEnabled;
        private long messageIndex;
        private string diskLogPath;
        private bool deleteLogFirst;
        private bool isDiskLoggingEnabled;
        private StreamWriter fileWriter;
        private string logHeader;

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
        
        public string GetFullLog()
        {
            lock (this.lockObject)
            {
                return this.logBuilder.ToString();
            }
        }

        public long GetLatestLogIndex()
        {
            lock (this.lockObject)
            {
                return this.messageIndex;
            }
        }

        public string GetLogFromIndex(int index)
        {
            StringBuilder log = new StringBuilder();
            lock (this.lockObject)
            {
                // Note messageIndex is not 0 based.
                for (int i = index; i < this.messageIndex; i++) 
                {
                    log.AppendLine(this.logMessages[i].Content);
                }
            }

            return log.ToString();
        }

        public void LogMessage(string content)
        {
            if (!this.isLoggingEnabled)
            {
                return;
            }

            lock (this.lockObject)
            {
                LogMessage msg = new LogMessage(content, this.messageIndex);
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
        }

        public void ConfigureLogging(LogHandlerConfig config)
        {
            this.isLoggingEnabled = true;
            this.logHeader = config.Header;
            this.LogMessage(config.Header);

            if (config.EnableDiskLogging)
            {
                this.EnableLoggingToDisk(config.LogFile, config.DeleteCurrentLogFirst);
            }
        }
        
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
            }
        }

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

        private void SetupLogHeader(string header)
        {
            this.logHeader = header;
            this.LogMessage(header);
        }

        private void EnableLoggingToDisk(string logFile, bool deleteCurrentLogFirst)
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

            this.LogMessage(e.Message);
        }

        private void HandBrakeUtils_MessageLogged(object sender, MessageLoggedEventArgs e)
        {
            if (e == null || string.IsNullOrEmpty(e.Message))
            {
                return;
            }

            this.LogMessage(e.Message);
        }
    }
}
