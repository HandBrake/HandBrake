// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RemoteScanInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Implementation of IScanInstance that works with a remote process rather than locally in-process.
//   This class is effectively just a shim.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text.Json;
    using System.Threading;
    using System.Threading.Tasks;

    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Interfaces.Model.Preview;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.Scan;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;
    using HandBrake.Worker.Routing.Commands;

    using HandBrakeWPF.Instance.Model;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    using Timer = System.Timers.Timer;

    public class RemoteScanInstance : RemoteBase, IScanInstance, IDisposable
    {
        private const double ScanPollIntervalMs = 250;
        private readonly object scanPollLockOb = new object();

        private Timer scanPollTimer;
        private int retryCount;
        private bool scanCompleteFired;

        public RemoteScanInstance(ILog logService, IUserSettingService userSettingService, IPortService portService)
            : base(logService, userSettingService, portService)
        {
        }

        public event EventHandler<EventArgs> ScanCompleted;

        public event EventHandler<ScanProgressEventArgs> ScanProgress;

        public int FeatureTitle { get; private set; }

        public JsonScanObject Titles { get; private set; }

        public RawPreviewData GetPreview(JsonEncodeObject job, int previewNumber)
        {
            if (this.IsServerRunning())
            {
                PreviewCommand previewCommand = new PreviewCommand() { EncodeSettings = job, PreviewNumber = previewNumber };
                string previewCommandJson = JsonSerializer.Serialize(previewCommand, JsonSettings.Options);

                var task = Task.Run(async () => await this.MakeHttpJsonPostRequest("GetPreview", previewCommandJson));
                task.Wait();

                if (task.IsCompleted && task.Result != null)
                {
                    ServerResponse response = task.Result;
                    if (response.WasSuccessful && !string.IsNullOrEmpty(response.JsonResponse))
                    {
                        RawPreviewData previewData = JsonSerializer.Deserialize<RawPreviewData>(response.JsonResponse, JsonSettings.Options);
                        return previewData;
                    }
                }
            }
            return null;
        }

        public void StartScan(List<string> paths, int previewCount, TimeSpan minDuration, int titleIndex, List<string> fileExclusionList, int hwDecode, bool keepDuplicateTitles)
        {
            if (this.IsServerRunning())
            {
                scanCompleteFired = false;
                Thread thread1 = new Thread(() => RunScanInitProcess(paths, previewCount, minDuration, titleIndex, hwDecode, keepDuplicateTitles));
                thread1.Start();
            }
            else
            {
                this.ScanCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(-10));
            }
        }

        public async void StopScan()
        {
            if (this.IsServerRunning())
            {
                await this.MakeHttpGetRequest("StopScan");
            }
        }

        public JsonState GetProgress()
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

        private void RunScanInitProcess(List<string> paths, int previewCount, TimeSpan minDuration, int titleIndex, int hwDecode, bool keepDuplicateTitles)
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
                    Mode = 2,
                    ExcludeExtnesionList = this.userSettingService.GetUserSetting<List<string>>(UserSettingConstants.ExcludedExtensions),
                    HwDecode = hwDecode,
                    KeepDuplicateTitles = keepDuplicateTitles
                };

                initCommand.LogFile = Path.Combine(initCommand.LogDirectory, string.Format("activity_log.worker.{0}.txt", GeneralUtilities.ProcessId));

                string job = JsonSerializer.Serialize(new ScanCommand { InitialiseCommand = initCommand, Paths = paths, MinDuration = minDuration, PreviewCount = previewCount, TitleIndex = titleIndex}, JsonSettings.Options);

                var task = Task.Run(async () => await this.MakeHttpJsonPostRequest("StartScan", job));
                task.Wait();
                this.MonitorEncodeProgress();
            }
        }

        private void MonitorEncodeProgress()
        {
            this.scanPollTimer = new Timer();
            this.scanPollTimer.Interval = ScanPollIntervalMs;

            this.scanPollTimer.Elapsed += (o, e) =>
            {
                try
                {
                    this.PollScanProgress();
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                }
            };
            this.scanPollTimer.Start();
        }

        private async void PollScanProgress()
        {
            lock (scanPollLockOb)
            {
                ProcessScanResult();
            }
        }

        private async void ProcessScanResult()
        {
            if (scanCompleteFired)
            {
                this.scanPollTimer?.Stop();
                this.scanPollTimer?.Dispose();
                return;
            }

            ServerResponse response = null;
            try
            {
                if (this.retryCount > 5)
                {
                    scanCompleteFired = true;
                    this.scanPollTimer?.Stop();

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

                    this.ScanCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(exitcode));

                    if (this.workerProcess != null && !this.workerProcess.HasExited)
                    {
                        this.workerProcess?.Kill();
                    }

                    return;
                }

                response = await this.MakeHttpGetRequest("PollScanProgress");
            }
            catch (Exception)
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

            if (string.IsNullOrEmpty(statusJson))
            {
                return; // Skip, We don't have data yet.
            }

            JsonState state = JsonSerializer.Deserialize<JsonState>(statusJson, JsonSettings.Options);

            TaskState taskState = state != null ? TaskState.FromRepositoryValue(state.State) : null;

            if (taskState != null && (taskState == TaskState.Scanning || taskState == TaskState.Searching))
            {
                if (this.ScanProgress != null)
                {
                    ScanProgressEventArgs progressEventArgs = new ScanProgressEventArgs(
                        state.Scanning.Progress,
                        state.Scanning.Preview,
                        state.Scanning.PreviewCount,
                        state.Scanning.Title,
                        state.Scanning.TitleCount);

                    this.ScanProgress(this, progressEventArgs);
                }
            }
            else if (taskState != null && taskState == TaskState.WorkDone) // Workdone here is deliberate. ScanDone in the worker doesn't mean the data is ready to call for.
            {
                this.scanPollTimer.Stop();
                scanCompleteFired = true;

                response = await this.MakeHttpGetRequest("GetTitles");
                if (response.WasSuccessful && !string.IsNullOrEmpty(response.JsonResponse))
                {
                    this.Titles = JsonSerializer.Deserialize<JsonScanObject>(
                        response.JsonResponse,
                        JsonSettings.Options);
                }

                response = await this.MakeHttpGetRequest("GetMainTitle");
                if (response.WasSuccessful && !string.IsNullOrEmpty(response.JsonResponse))
                {
                    this.FeatureTitle = int.TryParse(response.JsonResponse, out int featureTitle)
                        ? featureTitle
                        : -1;
                }

                this.ScanCompleted?.Invoke(sender: this, e: new EncodeCompletedEventArgs(0));

                // Note, we do not exit here. We can re-use this instance for the next scan
                // or for previews. 
            }
        }
    }
}
