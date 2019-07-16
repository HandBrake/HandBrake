// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RemoteInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Implementation of IEncodeInstance that works with a remote process rather than locally in-process.
//   This class is effectivly just a shim.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Net.Http;
    using System.Threading.Tasks;
    using System.Timers;

    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.State;

    using HandBrakeWPF.Instance.Model;

    using Newtonsoft.Json;

    /*
     * TODO:
     *  1. Add support for logging.
     *  2. Code to detect what ports are in use / select one within range.
     *  3. Add support for communciating via socket instead of HTTP.
     */

    public class RemoteInstance : IEncodeInstance, IDisposable
    {
        private const double EncodePollIntervalMs = 500;
        private readonly HttpClient client = new HttpClient();
        private readonly string serverUrl;
        private readonly int port;
        private Process workerProcess;
        private Timer encodePollTimer;

        public RemoteInstance(int port)
        {
            this.port = port;
            this.serverUrl = "http://localhost/";
        }

        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        public async void PauseEncode()
        {
            this.StopPollingProgres();
            await this.MakeHttpGetRequest("PauseEncode");
        }

        public async void ResumeEncode()
        {
            await this.MakeHttpGetRequest("ResumeEncode");
            this.MonitorEncodeProgress();
        }

        public async void StartEncode(JsonEncodeObject jobToStart)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
            string job = JsonConvert.SerializeObject(jobToStart, Formatting.Indented, settings);

            var values = new Dictionary<string, string> { { "jpb", job } };
            await this.MakeHttpPostRequest("StartEncode", values);

            this.MonitorEncodeProgress();
        }

        public async void StopEncode()
        {
            this.StopPollingProgres();
            await this.MakeHttpGetRequest("StopEncode");
        }

        public JsonState GetEncodeProgress()
        {
            Task<ServerResponse> response = this.MakeHttpGetRequest("PollEncodeProgress");
            response.Wait();

            if (!response.Result.WasSuccessful)
            {
                return null;
            }

            string statusJson = response.Result?.JsonResponse;

            JsonState state = JsonConvert.DeserializeObject<JsonState>(statusJson);
            return state;
        }

        public void Initialize(int verbosity, bool noHardware)
        {
            this.StartServer();
        }

        public void Dispose()
        {
            this.client?.Dispose();
            this.workerProcess?.Dispose();
            this.StopEncode();
            this.StopServer();
        }

        private void StartServer()
        {
            if (this.workerProcess == null || this.workerProcess.HasExited)
            {
                this.workerProcess = new Process();
                this.workerProcess.StartInfo =
                    new ProcessStartInfo("HandBrake.Worker.exe", string.Format("--port={0}", this.port))
                        {
                            WindowStyle = ProcessWindowStyle.Normal
                        };

                this.workerProcess.Start();
                this.workerProcess.Exited += this.WorkerProcess_Exited;
                Debug.WriteLine("Worker Process Started. PID = {0}", this.workerProcess.Id);
            }
        }

        private void MonitorEncodeProgress()
        {
            this.encodePollTimer = new Timer();
            this.encodePollTimer.Interval = EncodePollIntervalMs;

            this.encodePollTimer.Elapsed += (o, e) =>
                {
                    try
                    {
                        this.PollEncodeProgress();
                    }
                    catch (Exception exc)
                    {
                        Debug.WriteLine(exc);
                    }
                };
            this.encodePollTimer.Start();
        }

        private void StopPollingProgres()
        {
            this.encodePollTimer?.Stop();
        }

        private void WorkerProcess_Exited(object sender, EventArgs e)
        {
            Debug.WriteLine("Worker Process has exited");
        }

        private void StopServer()
        {
            if (this.workerProcess != null && !this.workerProcess.HasExited)
            {
                this.workerProcess.Kill();
            }
        }

        private async void PollEncodeProgress()
        {
            ServerResponse response = await this.MakeHttpGetRequest("PollEncodeProgress");
            if (!response.WasSuccessful)
            {
                return;
            }

            string statusJson = response.JsonResponse;

            JsonState state = JsonConvert.DeserializeObject<JsonState>(statusJson);

            TaskState taskState = state != null ? TaskState.FromRepositoryValue(state.State) : null;

            if (taskState != null && (taskState == TaskState.Working || taskState == TaskState.Muxing || taskState == TaskState.Searching))
            {
                if (this.EncodeProgress != null)
                {
                    var progressEventArgs = new EncodeProgressEventArgs(
                        fractionComplete: state.Working.Progress,
                        currentFrameRate: state.Working.Rate,
                        averageFrameRate: state.Working.RateAvg,
                        estimatedTimeLeft: TimeSpan.FromSeconds(state.Working.ETASeconds),
                        passId: state.Working.PassID,
                        pass: state.Working.Pass,
                        passCount: state.Working.PassCount,
                        stateCode: taskState.Code);

                    this.EncodeProgress(this, progressEventArgs);
                }
            }
            else if (taskState != null && taskState == TaskState.WorkDone)
            {
                this.encodePollTimer.Stop();

                this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(state.WorkDone.Error != 0));
            }
        }

        private async Task<ServerResponse> MakeHttpPostRequest(string urlPath, Dictionary<string, string> postValues)
        {
            if (postValues == null || !postValues.Any())
            {
                throw new InvalidOperationException("No Post Values Found.");
            }

            if (postValues.Any())
            {
                FormUrlEncodedContent content = new FormUrlEncodedContent(postValues);
                HttpResponseMessage response = await this.client.PostAsync(this.serverUrl + urlPath, content);
                if (response != null)
                {
                    string returnContent = await response.Content.ReadAsStringAsync();
                    ServerResponse serverResponse = new ServerResponse(response.IsSuccessStatusCode, returnContent);

                    return serverResponse;
                }
            }

            return null;
        }

        private async Task<ServerResponse> MakeHttpGetRequest(string urlPath)
        {
            HttpResponseMessage response = await this.client.GetAsync(this.serverUrl + urlPath);
            if (response != null)
            {
                string returnContent = await response.Content.ReadAsStringAsync();
                ServerResponse serverResponse = new ServerResponse(response.IsSuccessStatusCode, returnContent);

                return serverResponse;
            }

            return null;
        }
    }
}
