// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ApiRouter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   This is a service worker for the HandBrake app. It allows us to run encodes / scans in a separate process easily.
//   All API's expose the ApplicationServices models as JSON.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Routing
{
    using System;
    using System.Net;
    using System.Text.Json;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Interfaces.Model.Preview;
    using HandBrake.Interop.Interop.Json.Scan;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;
    using HandBrake.Worker.Logging;
    using HandBrake.Worker.Logging.Interfaces;
    using HandBrake.Worker.Routing.Commands;
    using HandBrake.Worker.Routing.Results;
    using HandBrake.Worker.Utilities;
    using HandBrake.Worker.Watcher;

    public class ApiRouter
    {
        private JsonState completedState;
        private JsonState completedScanState;
        private HandBrakeInstance handbrakeInstance;
        private ILogHandler logHandler;
        private InstanceWatcher instanceWatcher;

        public event EventHandler TerminationEvent;
        private readonly object stateLock = new object();
        
        public string GetVersionInfo(HttpListenerRequest request)
        {
            string versionInfo = JsonSerializer.Serialize(HandBrakeVersionHelper.GetVersion(), JsonSettings.Options);

            return versionInfo;
        }

        /* Encode API */

        public string StartEncode(HttpListenerRequest request)
        {
            this.completedState = null;
            string requestPostData = HttpUtilities.GetRequestPostData(request);

            if (!string.IsNullOrEmpty(requestPostData))
            {
                EncodeCommand command = JsonSerializer.Deserialize<EncodeCommand>(requestPostData, JsonSettings.Options);

                this.Initialise(command.InitialiseCommand);
                
                this.handbrakeInstance.StartEncode(command.EncodeJob);

                return JsonSerializer.Serialize(new CommandResult() { WasSuccessful = true }, JsonSettings.Options);
            }

            return JsonSerializer.Serialize(new CommandResult() { WasSuccessful = false, Error = "No POST data" }, JsonSettings.Options);
        }

        public string StopEncode(HttpListenerRequest request)
        {
            this.handbrakeInstance?.StopEncode();

            return (string)null;
        }

        public string PauseEncode(HttpListenerRequest request)
        {
            this.handbrakeInstance?.PauseEncode();

            return null;
        }

        public string ResumeEncode(HttpListenerRequest request)
        {
            this.handbrakeInstance?.ResumeEncode();

            return null;
        }

        public string PollEncodeProgress(HttpListenerRequest request)
        {
            if (this.completedState != null)
            {
                string json = JsonSerializer.Serialize(this.completedState, JsonSettings.Options);
                return json;
            }

            if (this.handbrakeInstance != null)
            {
                JsonState statusJson = this.handbrakeInstance.GetProgress();
                string json = JsonSerializer.Serialize(statusJson, JsonSettings.Options);

                return json;
            }

            return null;
        }


        /* Scan API */
        public string StartScan(HttpListenerRequest request)
        {
            this.completedScanState = null;
            string requestPostData = HttpUtilities.GetRequestPostData(request);

            if (!string.IsNullOrEmpty(requestPostData))
            {
                ScanCommand command = JsonSerializer.Deserialize<ScanCommand>(requestPostData, JsonSettings.Options);

                this.Initialise(command.InitialiseCommand);

                this.handbrakeInstance.StartScan(command.Path, command.PreviewCount, command.MinDuration, command.TitleIndex, command.InitialiseCommand.ExcludeExtnesionList);

                return JsonSerializer.Serialize(new CommandResult() { WasSuccessful = true }, JsonSettings.Options);
            }

            return JsonSerializer.Serialize(new CommandResult() { WasSuccessful = false, Error = "No POST data" }, JsonSettings.Options);
        }

        public string StopScan(HttpListenerRequest request)
        {
            this.handbrakeInstance?.StopScan();
            return (string)null;
        }

        public string PollScanProgress(HttpListenerRequest request)
        {
            lock (this.stateLock)
            {
                if (this.completedScanState != null)
                {
                    string json = JsonSerializer.Serialize(this.completedScanState, JsonSettings.Options);
                    return json;
                }

                if (this.handbrakeInstance != null)
                {
                    JsonState statusJson = this.handbrakeInstance.GetProgress();
                    string json = JsonSerializer.Serialize(statusJson, JsonSettings.Options);
                    return json;
                }

                return null;
            }
        }

        public string GetScanTitles(HttpListenerRequest request)
        {
            lock (this.stateLock)
            {
                if (this.handbrakeInstance != null)
                {
                    JsonScanObject scanObj = this.handbrakeInstance.Titles;
                    string json = JsonSerializer.Serialize(scanObj, JsonSettings.Options);
                    return json;
                }
                return null;
            }
        }

        public string GetMainScanTitle(HttpListenerRequest request)
        {
            lock (this.stateLock)
            {
                if (this.handbrakeInstance != null)
                {
                    int scanObj = this.handbrakeInstance.FeatureTitle;
                    string json = JsonSerializer.Serialize(scanObj, JsonSettings.Options);

                    return json;
                }

                return null;
            }
        }

        public string GetPreview(HttpListenerRequest request)
        {
            lock (this.stateLock)
            {
                string requestPostData = HttpUtilities.GetRequestPostData(request);

                if (this.handbrakeInstance != null)
                {
                    PreviewCommand command = JsonSerializer.Deserialize<PreviewCommand>(requestPostData, JsonSettings.Options);
                    RawPreviewData image = this.handbrakeInstance.GetPreview(command.EncodeSettings, command.PreviewNumber);

                    string json = JsonSerializer.Serialize(image, JsonSettings.Options);

                    return json;
                }

                return null;
            }
        }

        /* Logging API */

        // GET
        public string GetAllLogMessages(HttpListenerRequest request)
        {
            return JsonSerializer.Serialize(this.logHandler.GetLogMessages(), JsonSettings.Options);
        }
        
        // POST
        public string GetLogMessagesFromIndex(HttpListenerRequest request)
        {
            string requestPostData = HttpUtilities.GetRequestPostData(request);

            if (int.TryParse(requestPostData, out int index))
            {
                return JsonSerializer.Serialize(this.logHandler.GetLogMessagesFromIndex(index), JsonSettings.Options);
            }

            return null;
        }

        public string ResetLogging(HttpListenerRequest request)
        {
            this.logHandler.Reset();

            return null;
        }
        
        /* Helper Methods */

        public void OnTerminationEvent()
        {
            this.TerminationEvent?.Invoke(this, EventArgs.Empty);
        }

        private void Initialise(InitCommand command)
        {
            if (this.handbrakeInstance == null)
            {
                this.handbrakeInstance = new HandBrakeInstance();
            }

            if (this.logHandler == null)
            {
                this.logHandler = new LogHandler(command.LogDirectory, command.LogFile, command.EnableDiskLogging);
            }

            if (!command.AllowDisconnectedWorker)
            {
                ConsoleOutput.WriteLine("Worker: Disconnected worker monitoring enabled!", ConsoleColor.White, true);
                this.instanceWatcher = new InstanceWatcher(this);
                this.instanceWatcher.Start(5000);
            }

            this.completedState = null;

            this.handbrakeInstance.Initialize(command.LogVerbosity, !command.EnableHardwareAcceleration);
            this.handbrakeInstance.EncodeCompleted += this.HandbrakeInstance_EncodeCompleted;
            this.handbrakeInstance.ScanCompleted += this.HandbrakeInstance_ScanCompleted;

            if (command.EnableLibDvdNav)
            {
                HandBrakeUtils.SetDvdNav(true);
            }
        }
        
        /* Event Handlers */

        private void HandbrakeInstance_ScanCompleted(object sender, EventArgs e)
        {
            lock (this.stateLock)
            {
                this.completedScanState = new JsonState() { WorkDone = new WorkDone() { Error = 0 } };
                this.completedScanState.State = TaskState.WorkDone.Code;
            }
        }

        private void HandbrakeInstance_EncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            lock (this.stateLock)
            {
                this.completedState = new JsonState() { WorkDone = new WorkDone() { Error = e.Error } };
                this.completedState.State = TaskState.WorkDone.Code;
                this.logHandler.ShutdownFileWriter();
                this.handbrakeInstance.Dispose();
                HandBrakeUtils.DisposeGlobal();
            }
        }
    }
}
