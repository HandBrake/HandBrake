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
        private HandBrakeInstance handbrakeInstance;
        private ILogHandler logHandler;
        private InstanceWatcher instanceWatcher;

        public event EventHandler TerminationEvent; 

        public string GetVersionInfo(HttpListenerRequest request)
        {
            string versionInfo = JsonSerializer.Serialize(HandBrakeVersionHelper.GetVersion(), JsonSettings.Options);

            return versionInfo;
        }

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
                JsonState statusJson = this.handbrakeInstance.GetEncodeProgress();
                string json = JsonSerializer.Serialize(statusJson, JsonSettings.Options);

                return json;
            }

            return null;
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

        public void OnTerminationEvent()
        {
            this.TerminationEvent?.Invoke(this, EventArgs.Empty);
        }

        private void HandbrakeInstance_EncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.completedState = new JsonState() { WorkDone = new WorkDone() { Error = e.Error } };
            this.completedState.State = "WORKDONE";
            this.logHandler.ShutdownFileWriter();
            this.handbrakeInstance.Dispose();
            HandBrakeUtils.DisposeGlobal();
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

            if (command.EnableLibDvdNav)
            {
                HandBrakeUtils.SetDvdNav(true);
            }
        }
    }
}
