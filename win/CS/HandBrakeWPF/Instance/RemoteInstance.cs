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
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Net.NetworkInformation;
    using System.Text;
    using System.Threading.Tasks;
    using System.Timers;
    using System.Windows.Media.Animation;


    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Model;
    using HandBrake.Worker.Routing.Commands;

    using HandBrakeWPF.Instance.Model;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Services;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;

    public class RemoteInstance : HttpRequestBase, IEncodeInstance, IDisposable
    {
        private readonly HBConfiguration configuration;
        private readonly ILog logService;
        private readonly IUserSettingService userSettingService;

        private const double EncodePollIntervalMs = 500;

        private Process workerProcess;
        private Timer encodePollTimer;
        private int retryCount = 0;

        public RemoteInstance(HBConfiguration configuration, ILog logService, IUserSettingService userSettingService)
        {
            this.configuration = configuration;
            this.logService = logService;
            this.userSettingService = userSettingService;
            this.port = this.GetOpenPort(userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessIsolationPort));
            this.serverUrl = string.Format("http://127.0.0.1:{0}/", this.port);
        }

        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        public bool IsRemoteInstance => true;

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
            InitCommand initCommand = new InitCommand
            {
                EnableDiskLogging = false,
                AllowDisconnectedWorker = false,
                DisableLibDvdNav = !this.userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav),
                EnableHardwareAcceleration = true,
                LogDirectory = DirectoryUtilities.GetLogDirectory(),
                LogVerbosity = this.userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity)
            };

            initCommand.LogFile = Path.Combine(initCommand.LogDirectory, string.Format("activity_log.worker.{0}.txt", GeneralUtilities.ProcessId));

            JsonSerializerSettings settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
            string job = JsonConvert.SerializeObject(new EncodeCommand { InitialiseCommand = initCommand, EncodeJob = jobToStart }, Formatting.None, settings);

            await this.MakeHttpJsonPostRequest("StartEncode", job);

            this.MonitorEncodeProgress();
        }

        public async void StopEncode()
        {
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

        public void Initialize(int verbosityLvl, bool noHardwareMode)
        {
            this.StartServer();
        }

        public void Dispose()
        {
            this.workerProcess?.Dispose();
        }

        private async void StartServer()
        {
            if (this.workerProcess == null || this.workerProcess.HasExited)
            {
                var plainTextBytes = Encoding.UTF8.GetBytes(Guid.NewGuid().ToString());
                this.base64Token = Convert.ToBase64String(plainTextBytes);

                workerProcess = new Process
                                {
                                    StartInfo =
                                    {
                                        FileName = "HandBrake.Worker.exe",
                                        Arguments = string.Format(" --port={0} --token={1}", port, this.base64Token),
                                        UseShellExecute = false,
                                        RedirectStandardOutput = true,
                                        RedirectStandardError = true,
                                        CreateNoWindow = true
                                    }
                                };
                workerProcess.Exited += this.WorkerProcess_Exited;
                workerProcess.OutputDataReceived += this.WorkerProcess_OutputDataReceived;
                workerProcess.ErrorDataReceived += this.WorkerProcess_OutputDataReceived;

                workerProcess.Start();
                workerProcess.BeginOutputReadLine();
                workerProcess.BeginErrorReadLine();

                // Set Process Priority
                switch ((ProcessPriority)this.userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessPriorityInt))
                {
                    case ProcessPriority.High:
                        workerProcess.PriorityClass = ProcessPriorityClass.High;
                        break;
                    case ProcessPriority.AboveNormal:
                        workerProcess.PriorityClass = ProcessPriorityClass.AboveNormal;
                        break;
                    case ProcessPriority.Normal:
                        workerProcess.PriorityClass = ProcessPriorityClass.Normal;
                        break;
                    case ProcessPriority.Low:
                        workerProcess.PriorityClass = ProcessPriorityClass.Idle;
                        break;
                    default:
                        workerProcess.PriorityClass = ProcessPriorityClass.BelowNormal;
                        break;
                }

                this.logService.LogMessage(string.Format("Worker Process started with Process ID: {0} and port: {1}", this.workerProcess.Id, port));
            }
        }

        private void WorkerProcess_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            this.logService.LogMessage(e.Data);
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
            this.PollEncodeProgress(); // Get the last progress state.
            this.encodePollTimer?.Stop();
        }

        private void WorkerProcess_Exited(object sender, EventArgs e)
        {
            this.logService.LogMessage("Worker Process Existed!");
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
                if (this.retryCount > 5)
                {
                    this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(4));

                    this.encodePollTimer?.Stop();

                    if (this.workerProcess != null && !this.workerProcess.HasExited)
                    {
                        this.workerProcess?.Kill();
                    }

                    return;
                }

                response = await this.MakeHttpGetRequest("PollEncodeProgress");
            }
            catch (Exception e)
            {
                retryCount = this.retryCount + 1;
            }

            if (response == null || !response.WasSuccessful)
            {
                retryCount = this.retryCount + 1;
                return;
            }

            this.retryCount = 0; // Reset

            string statusJson = response.JsonResponse;

            JsonState state = JsonConvert.DeserializeObject<JsonState>(statusJson);

            TaskState taskState = state != null ? TaskState.FromRepositoryValue(state.State) : null;

            if (taskState != null && (taskState == TaskState.Working || taskState == TaskState.Searching))
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
                if (this.workerProcess != null && !this.workerProcess.HasExited)
                {
                    this.workerProcess?.Kill();
                }

                this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(state.WorkDone.Error));
            }
        }

        private int GetOpenPort(int startPort)
        {
            if (startPort == 0)
            {
                startPort = 8037;
            }

            int portStartIndex = startPort;

            IPGlobalProperties properties = IPGlobalProperties.GetIPGlobalProperties();
            IPEndPoint[] tcpEndPoints = properties.GetActiveTcpListeners();

            List<int> usedPorts = tcpEndPoints.Select(p => p.Port).ToList<int>();
            int unusedPort = 0;

            unusedPort = Enumerable.Range(portStartIndex, 99).FirstOrDefault(p => !usedPorts.Contains(p));

            if (startPort != unusedPort)
            {
                this.logService.LogMessage(string.Format("Port {0} in use. Using {1} instead", startPort, unusedPort));
            }

            return unusedPort;
        }
    }
}
