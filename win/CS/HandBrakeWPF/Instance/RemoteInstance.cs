// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RemoteInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Implementation of IEncodeInstance that works with a remote process rather than locally in-process.
//   This class is effectively just a shim.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Net.Http;
    using System.Net.Http.Headers;
    using System.Runtime.CompilerServices;
    using System.Text;
    using System.Threading.Tasks;
    using System.Timers;

    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.State;

    using HandBrakeWPF.Instance.Model;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;

    /*
     * TODO:
     *  - Handle Worker Shutdown.
     *  - Worker Registration Process
     *  - Port in Use Handling
     *  - Setting Configuration (libdvdnav)
     *  - Setting No Hardware mode
     */

    public class RemoteInstance : HttpRequestBase, IEncodeInstance, IDisposable
    {
        private const double EncodePollIntervalMs = 1000;

        private Process workerProcess;
        private Timer encodePollTimer;

        public RemoteInstance(int port)
        {
            this.port = port;
            this.serverUrl = string.Format("http://127.0.0.1:{0}/", this.port);
        }

        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        public async void PauseEncode()
        {
            await this.MakeHttpGetRequest("PauseEncode");
            this.StopPollingProgress();
        }

        public async void ResumeEncode()
        {
            await this.MakeHttpGetRequest("ResumeEncode");
            this.MonitorEncodeProgress();
        }

        public async void StartEncode(JsonEncodeObject jobToStart)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
            string job = JsonConvert.SerializeObject(jobToStart, Formatting.None, settings);

            await this.MakeHttpJsonPostRequest("StartEncode", job);

            this.MonitorEncodeProgress();
        }

        public async void StopEncode()
        {
            await this.MakeHttpGetRequest("StopEncode");
            this.StopPollingProgress();
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

        public void Initialize(int verbosityLvl, bool noHardwareMode)
        {
            this.StartServer(verbosityLvl, noHardwareMode);
        }

        public void Dispose()
        {
            this.client?.Dispose();
            this.workerProcess?.Dispose();
            this.StopEncode();
            this.StopServer();
        }

        private void StartServer(int verbosityLvl, bool noHardwareMode)
        {
            if (this.workerProcess == null || this.workerProcess.HasExited)
            {
                this.workerProcess = new Process();
                this.workerProcess.StartInfo =
                    new ProcessStartInfo("HandBrake.Worker.exe", string.Format("--port={0} --verbosity={1}", this.port, verbosityLvl))
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

        private void StopPollingProgress()
        {
            this.PollEncodeProgress(); // Get the final progress state.
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
            ServerResponse response = null;
            try
            {
                response = await this.MakeHttpGetRequest("PollEncodeProgress");
            }
            catch (Exception e)
            {
                if (this.encodePollTimer != null)
                {
                    this.encodePollTimer.Stop();
                }
            }

            if (response == null || !response.WasSuccessful)
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
    }
}
