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
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Text.Json;
    using System.Threading;
    using System.Threading.Tasks;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;
    using HandBrake.Worker.Routing.Commands;

    using HandBrakeWPF.Instance.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    using EncodeCommand = HandBrakeWPF.Model.Worker.EncodeCommand;
    using Timer = System.Timers.Timer;

    public class RemoteInstance : RemoteBase, IEncodeInstance, IDisposable
    {
        private const double EncodePollIntervalMs = 500;

        private Timer encodePollTimer;
        private int retryCount;
        private bool encodeCompleteFired;
        private object lockObject = new object();

        public RemoteInstance(ILog logService, IUserSettingService userSettingService, IPortService portService) : base(logService, userSettingService, portService)
        {
        }

        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

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

        public JsonState GetProgress()
        {
            throw new NotImplementedException("Not Used");
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
                        lock (lockObject)
                        {
                            this.PollEncodeProgress();
                        }
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
            catch (Exception e)
            {
                retryCount = this.retryCount + 1;

                if (retryCount > 5)
                {
                    this.ServiceLogMessage("Worker: Final attempt to communicate failed: " + e);
                }
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

            if (string.IsNullOrEmpty(statusJson))
            {
                retryCount = this.retryCount + 1;
                this.encodePollTimer?.Start(); // Reset and try again.
                return;
            }

            try
            {
                JsonState state = JsonSerializer.Deserialize<JsonState>(statusJson, JsonSettings.Options);
                if (state != null)
                {
                    ProcessStateChange(state);
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e); // Silently ignore.
            }

            // Next Run.
            this.encodePollTimer?.Start();
        }

        private void ProcessStateChange(JsonState state)
        {
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
                    LogVerbosity = this.userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity),
                    Mode = 1,
                    ExcludeExtnesionList = this.userSettingService.GetUserSetting<List<string>>(UserSettingConstants.ExcludedExtensions)
                };

                initCommand.LogFile = Path.Combine(initCommand.LogDirectory, string.Format("activity_log.worker.{0}.txt", GeneralUtilities.ProcessId));

                bool startRequested = false;
                try
                {
                    string job = JsonSerializer.Serialize(new EncodeCommand { InitialiseCommand = initCommand, EncodeJob = jobToStart }, JsonSettings.Options);

                    var task = Task.Run(async () => await this.MakeHttpJsonPostRequest("StartEncode", job));
                    task.Wait();
                    startRequested = true;
                }
                catch (Exception exc)
                {
                    startRequested = false;
                    this.ServiceLogMessage("Unable to start job. HandBrake was unable to communicate with the worker process. This may be getting blocked by security software. Try running without process isolation. See Tools Menu -> Preferences -> Advanced." + Environment.NewLine + exc.ToString());
                    this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(4));
                    return;
                }

                try
                {
                    if (startRequested)
                    {
                        this.MonitorEncodeProgress();
                    }
                }
                catch (Exception exc)
                {
                    this.ServiceLogMessage(exc.ToString());
                    this.EncodeCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(4));
                }
            }
        }
    }
}
