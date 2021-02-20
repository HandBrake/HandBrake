// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogHandler.cs" company="HandBrake Project (http://handbrake.fr)">
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

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Worker.Logging.Interfaces;
    using HandBrake.Worker.Logging.Models;

    public class LogHandler : ILogHandler
    {
        private readonly object lockObject = new object();
        private readonly object fileWriterLock = new object();

        private readonly StringBuilder logBuilder = new StringBuilder();
        private readonly List<LogMessage> logMessages = new List<LogMessage>();

        private bool isLoggingEnabled = true;
        private bool isDiskLoggingEnabled = false;
        private int messageIndex;
        private StreamWriter fileWriter;

        public LogHandler(string logDirectory, string logFileName, bool enableDiskLogging)
        {
            this.isDiskLoggingEnabled = enableDiskLogging;

            if (this.isDiskLoggingEnabled)
            {
                lock (this.fileWriterLock)
                {
                    // Todo Handle no log directory.
                    this.fileWriter = new StreamWriter(logFileName) { AutoFlush = true };
                }
            }

            HandBrakeUtils.MessageLogged += this.HandBrakeUtils_MessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeUtils_ErrorLogged;
        }

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

        public void LogMessage(string content)
        {
            Console.WriteLine(content);

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
        }

        public void Reset()
        {
            lock (this.lockObject)
            {
                this.logMessages.Clear();
                this.logBuilder.Clear();
                this.messageIndex = 0;
            }
        }

        public void ShutdownFileWriter()
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
