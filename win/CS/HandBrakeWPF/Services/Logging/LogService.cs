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
    using System.Timers;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Worker.Logging.Models;

    using HandBrakeWPF.Instance.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Model;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;

    using ILog = Interfaces.ILog;
    using LogEventArgs = EventArgs.LogEventArgs;

    public class LogService : HttpRequestBase, ILog
    {
        // TODO List.
        // Maybe make the event weak?
        // Make this class Thread Safe.
        private static ILog loggerInstance;
        private readonly object lockObject = new object();
        private readonly object fileWriterLock = new object();
        private readonly StringBuilder logBuilder = new StringBuilder();
 
        private bool isLoggingEnabled;
        private List<LogMessage> logMessages = new List<LogMessage>(); 
        private int messageIndex;
        private string diskLogPath;
        private bool deleteLogFirst;
        private bool isDiskLoggingEnabled;
        private StreamWriter fileWriter;
        private string logHeader;
        private Timer remoteLogPollTimer;
        private int remoteIndex = 0;
        private bool isRemotePollingEnabled = false;

        public LogService(IUserSettingService userSettingService)
        {
            HandBrakeUtils.MessageLogged += this.HandBrakeUtils_MessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeUtils_ErrorLogged;

            if (userSettingService.GetUserSetting<bool>(UserSettingConstants.RemoteServiceEnabled))
            {
                this.ActivateRemoteLogPolling();
                this.isRemotePollingEnabled = true;

                this.port = userSettingService.GetUserSetting<int>(UserSettingConstants.RemoteServicePort);
                this.serverUrl = string.Format("http://127.0.0.1:{0}/", this.port);
            }
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

        public void ConfigureLogging(LogHandlerConfig config)
        {
            this.isLoggingEnabled = true;

            if (config.EnableDiskLogging)
            {
                if (!string.IsNullOrEmpty(config.LogFile) && !Directory.Exists(Path.GetDirectoryName(config.LogFile)))
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(config.LogFile));
                }

                this.EnableLoggingToDisk(config.LogFile, config.DeleteCurrentLogFirst);
            }

            this.logHeader = config.Header;
            this.LogMessage(config.Header);
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

        public long GetLatestLogIndex()
        {
            lock (this.lockObject)
            {
                return this.messageIndex;
            }
        }
        
        public async void Reset()
        {
            //if (this.isRemotePollingEnabled)
            //{
            //    try
            //    {
            //        await this.MakeHttpGetRequest("ResetLogging");
            //    }
            //    catch (Exception e)
            //    {
            //        if (this.remoteLogPollTimer != null)
            //        {
            //            this.remoteLogPollTimer.Stop();
            //        }
            //    }
            //}

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

        protected virtual void OnMessageLogged(LogMessage msg)
        {
            var onMessageLogged = this.MessageLogged;
            if (onMessageLogged != null)
            {
                onMessageLogged.Invoke(this, new LogEventArgs(msg));
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

        protected virtual void OnLogReset()
        {
            this.LogReset?.Invoke(this, System.EventArgs.Empty);
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

        private void ActivateRemoteLogPolling()
        {
            this.remoteLogPollTimer = new Timer();
            this.remoteLogPollTimer.Interval = 1000;

            this.remoteLogPollTimer.Elapsed += (o, e) =>
            {
                try
                {
                    this.PollRemoteLog();
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                }
            };
            this.remoteLogPollTimer.Start();
        }

        private async void PollRemoteLog()
        {
            ServerResponse response = null;
            try
            {
                int nextIndex = this.remoteIndex + 1;
                string json = JsonConvert.SerializeObject(nextIndex, Formatting.Indented, this.jsonNetSettings);

                response = await this.MakeHttpJsonPostRequest("GetLogMessagesFromIndex", json);
            }
            catch (Exception e)
            {
                Debug.WriteLine("No Endpoint");
            }

            if (response == null || !response.WasSuccessful)
            {
                return;
            }

            string statusJson = response.JsonResponse;

            List<LogMessage> messages = null;
            if (!string.IsNullOrEmpty(statusJson))
            {
                messages = JsonConvert.DeserializeObject<List<LogMessage>>(statusJson, this.jsonNetSettings);
            }

            if (messages != null)
            {
                foreach (var item in messages)
                {
                    this.LogMessage(item.Content);
                    this.remoteIndex = item.MessageIndex;
                }
            }
        }
    }
}
