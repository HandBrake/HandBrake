// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ApiRouter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   This is a service worker for the HandBrake app. It allows us to run encodes / scans in a seperate process easily.
//   All API's expose the ApplicationServices models as JSON.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Routing
{
    using System;
    using System.Net;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;
    using HandBrake.Worker.Logging;
    using HandBrake.Worker.Logging.Interfaces;
    using HandBrake.Worker.Routing.Commands;
    using HandBrake.Worker.Routing.Results;
    using HandBrake.Worker.Utilities;
    using HandBrake.Worker.Watcher;

    using Newtonsoft.Json;

    public class ApiRouter
    {
        private readonly string token = Guid.NewGuid().ToString();
        private readonly JsonSerializerSettings jsonNetSettings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };

        private JsonState completedState;
        private HandBrakeInstance handbrakeInstance;
        private ILogHandler logHandler;
        private InstanceWatcher instanceWatcher;

        public event EventHandler TerminationEvent; 

        public string GetInstanceToken(HttpListenerRequest request)
        {
            return JsonConvert.SerializeObject(token, Formatting.Indented, this.jsonNetSettings);
        }

        public string GetVersionInfo(HttpListenerRequest request)
        {
            string versionInfo = JsonConvert.SerializeObject(VersionHelper.GetVersion(), Formatting.Indented, this.jsonNetSettings);

            return versionInfo;
        }

        public string StartEncode(HttpListenerRequest request)
        {
            this.completedState = null;
            string requestPostData = HttpUtilities.GetRequestPostData(request);

            if (!string.IsNullOrEmpty(requestPostData))
            {
                EncodeCommand command = JsonConvert.DeserializeObject<EncodeCommand>(requestPostData);

                this.Initialise(command.InitialiseCommand);
                
                this.handbrakeInstance.StartEncode(command.EncodeJob);

                return JsonConvert.SerializeObject(new CommandResult() { WasSuccessful = true }, Formatting.Indented, this.jsonNetSettings);
            }

            return JsonConvert.SerializeObject(new CommandResult() { WasSuccessful = false, Error = "No POST data" }, Formatting.Indented, this.jsonNetSettings);
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
                string json = JsonConvert.SerializeObject(this.completedState, Formatting.Indented, this.jsonNetSettings);
                return json;
            }

            if (this.handbrakeInstance != null)
            {
                JsonState statusJson = this.handbrakeInstance.GetEncodeProgress();
                string json = JsonConvert.SerializeObject(statusJson, Formatting.Indented, this.jsonNetSettings);

                return json;
            }

            return null;
        }

        /* Logging API */
        
        // GET
        public string GetAllLogMessages(HttpListenerRequest request)
        {
            return JsonConvert.SerializeObject(this.logHandler.GetLogMessages(), Formatting.Indented, this.jsonNetSettings);
        }
        
        // POST
        public string GetLogMessagesFromIndex(HttpListenerRequest request)
        {
            string requestPostData = HttpUtilities.GetRequestPostData(request);

            if (int.TryParse(requestPostData, out int index))
            {
                return JsonConvert.SerializeObject(this.logHandler.GetLogMessagesFromIndex(index), Formatting.Indented, this.jsonNetSettings);
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

        private void HandbrakeInstance_EncodeCompleted(object sender, Interop.Interop.EventArgs.EncodeCompletedEventArgs e)
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
                this.instanceWatcher = new InstanceWatcher(this);
                this.instanceWatcher.Start(5000);
            }

            this.completedState = null;

            this.handbrakeInstance.Initialize(command.LogVerbosity, !command.EnableHardwareAcceleration);
            this.handbrakeInstance.EncodeCompleted += this.HandbrakeInstance_EncodeCompleted;

            if (command.DisableLibDvdNav)
            {
                HandBrakeUtils.SetDvdNav(true); // TODO check this is correct
            }
        }
    }
}
