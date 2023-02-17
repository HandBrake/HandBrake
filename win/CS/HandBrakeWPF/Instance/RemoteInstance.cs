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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Text.Json;
    using System.Threading;
    using System.Threading.Tasks;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Instance.Model;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Model.Worker;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    using Timer = System.Timers.Timer;

    public class RemoteInstance : HttpRequestBase, IEncodeInstance, IDisposable
    {
        private readonly ILog logService;
        private readonly IUserSettingService userSettingService;
        private readonly IPortService portService;

        private const double EncodePollIntervalMs = 500;

        private Process workerProcess;
        private Timer encodePollTimer;
        private int retryCount;
        private bool encodeCompleteFired;
        private bool serverStarted;

        public RemoteInstance(ILog logService, IUserSettingService userSettingService, IPortService portService)
        {
            this.logService = logService;
            this.userSettingService = userSettingService;
            this.portService = portService;
        }

        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        public bool IsRemoteInstance => true;

        public async void PauseEncode()
        {
            if (this.IsServerRunning())
            {
                await this.MakeHttpGetRequest("PauseEncode");
                this.StopPollingProgress();
            }
        }

        public async void ResumeEncode()
        {
            if (this.IsServerRunning())
            {
                await this.MakeHttpGetRequest("ResumeEncode");
                this.MonitorEncodeProgress();
            }
        }

        public void StartEncode(JsonEncodeObject jobToStart)
        {
            if (this.IsServerRunning())
            {
                Thread thread1 = new Thread(() => RunEncodeInitProcess(jobToStart));
                thread1.Start();
            }
            else
            {
                this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(-10));
            }
        }

        public async void StopEncode()
        {
            if (this.IsServerRunning())
            {
                await this.MakeHttpGetRequest("StopEncode");
            }
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

            JsonState state = JsonSerializer.Deserialize<JsonState>(statusJson, JsonSettings.Options);
            return state;
        }

        public void Initialize(int verbosityLvl, bool noHardwareMode)
        {
            try
            {
                if (this.workerProcess == null || this.workerProcess.HasExited)
                {
                    var plainTextBytes = Encoding.UTF8.GetBytes(Guid.NewGuid().ToString());
                    this.base64Token = Convert.ToBase64String(plainTextBytes);
                    this.port = this.portService.GetOpenPort(userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessIsolationPort));
                    this.serverUrl = string.Format("http://127.0.0.1:{0}/", this.port);

                    workerProcess = new Process
                                    {
                                        StartInfo =
                                        {
                                            FileName = "HandBrake.Worker.exe",
                                            Arguments =
                                                string.Format(" --port={0} --token={1}", port, this.base64Token),
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

                    int maxAllowed = userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);
                    this.ServiceLogMessage(string.Format("Remote Process started with Process ID: {0} using port: {1}. Max Allowed Instances: {2}", this.workerProcess.Id, port, maxAllowed));
                }
            }
            catch (Exception e)
            {
                this.ServiceLogMessage("Unable to start worker process.");
                this.ServiceLogMessage(e.ToString());
            }
        }

        public void Dispose()
        {
            this.workerProcess?.Dispose();
        }

        private void WorkerProcess_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            this.logService.LogMessage(e.Data);
        }

        private void MonitorEncodeProgress()
        {
            this.encodePollTimer = new Timer();
            this.encodePollTimer.Interval = EncodePollIntervalMs;
            this.encodePollTimer.AutoReset = false;

            this.encodePollTimer.Elapsed += (o, e) =>
                {
                    try
                    {
                        this.PollEncodeProgress();
                    }
                    catch (Exception exc)
                    {
                        if (this.encodePollTimer != null)
                        {
                            this.encodePollTimer.Start();
                        }
                        Debug.WriteLine(exc);
                    }
                };
            this.encodePollTimer.Start();
        }

        private void StopPollingProgress()
        {
            try
            {
                this.PollEncodeProgress(); // Get the last progress state.
            }
            catch (Exception exc)
            {
                if (this.encodePollTimer != null)
                {
                    this.encodePollTimer.Start();
                }
                Debug.WriteLine(exc);
            }
           
            this.encodePollTimer?.Stop();
        }

        private void WorkerProcess_Exited(object sender, EventArgs e)
        {
            this.ServiceLogMessage("Worker process exited!");
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
            if (encodeCompleteFired)
            {
                this.encodePollTimer?.Stop();
                this.encodePollTimer?.Dispose();
                this.encodePollTimer = null;
                return;
            }

            ServerResponse response = null;
            try
            {
                if (this.retryCount > 5)
                {
                    encodeCompleteFired = true;
                    this.encodePollTimer?.Stop();

                    int exitcode = -11;
                    if (this.workerProcess != null && this.workerProcess.HasExited)
                    {
                        this.ServiceLogMessage("Worker process exit was not expected.");
                        exitcode = -12;
                    }
                    else
                    {
                        this.ServiceLogMessage("Worker process appears to be unresponsive. Terminating .... ");
                    }

                    this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(exitcode));
                    
                    if (this.workerProcess != null && !this.workerProcess.HasExited)
                    {
                        this.workerProcess?.Kill();
                    }

                    return;
                }

                response = await this.MakeHttpGetRequest("PollEncodeProgress");
            }
            catch (Exception)
            {
                retryCount = this.retryCount + 1;
            }

            if (response == null || !response.WasSuccessful)
            {
                retryCount = this.retryCount + 1;

                // Next Run.
                this.encodePollTimer?.Start();

                return;
            }

            this.retryCount = 0; // Reset

            string statusJson = response.JsonResponse;

            JsonState state = JsonSerializer.Deserialize<JsonState>(statusJson, JsonSettings.Options);

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
                this.encodePollTimer?.Stop();
                encodeCompleteFired = true;

                if (this.workerProcess != null && !this.workerProcess.HasExited)
                {
                    try
                    {
                        this.workerProcess?.Kill();
                    }
                    catch (Win32Exception e)
                    {
                        Debug.WriteLine(e);
                    }
                }

                this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(state.WorkDone.Error));
                this.portService.FreePort(this.port);
            }

            // Next Run.
            this.encodePollTimer?.Start();
        }

        private void RunEncodeInitProcess(JsonEncodeObject jobToStart)
        {
            if (this.IsServerRunning())
            {
                InitCommand initCommand = new InitCommand
                                      {
                                          EnableDiskLogging = false,
                                          AllowDisconnectedWorker = false,
                                          EnableLibDvdNav = !this.userSettingService.GetUserSetting<bool>(UserSettingConstants.DisableLibDvdNav),
                                          EnableHardwareAcceleration = true,
                                          LogDirectory = DirectoryUtilities.GetLogDirectory(),
                                          LogVerbosity = this.userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity)
                                      };

                initCommand.LogFile = Path.Combine(initCommand.LogDirectory, string.Format("activity_log.worker.{0}.txt", GeneralUtilities.ProcessId));

                string job = JsonSerializer.Serialize(new EncodeCommand { InitialiseCommand = initCommand, EncodeJob = jobToStart }, JsonSettings.Options);

                var task = Task.Run(async () => await this.MakeHttpJsonPostRequest("StartEncode", job));
                task.Wait();
                this.MonitorEncodeProgress();
            }
        }

        private bool IsServerRunning()
        {
            // Poll the server until it's started up. This allows us to prevent failures in upstream methods.
            if (this.serverStarted)
            {
                return this.serverStarted;  
            }

            int count = 0;
            while (!this.serverStarted)
            {
                if (count > 10)
                {
                    logService.LogMessage("Unable to connect to the HandBrake Worker instance after 10 attempts. Try disabling this option in Tools -> Preferences -> Advanced.");
                    return false;
                }

                try
                {
                    var task = Task.Run(async () => await this.MakeHttpGetRequest("IsTokenSet"));
                    task.Wait(2000);

                    if (string.Equals(task.Result.JsonResponse, "True", StringComparison.CurrentCultureIgnoreCase))
                    {
                        this.serverStarted = true;
                        return true;
                    }
                }
                catch (Exception)
                {
                    // Do nothing. We'll try again. The service isn't ready yet.
                }
                finally
                {
                    count = count + 1;
                }
            }

            return true;
        }

        private void ServiceLogMessage(string text)
        {
            string time = DateTime.Now.ToString("HH:mm:ss", System.Globalization.DateTimeFormatInfo.InvariantInfo);
            logService.LogMessage(string.Format("[{0}] {1}", time, text));
        }
    }
}
