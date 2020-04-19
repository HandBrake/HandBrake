// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LibEncode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   LibHB Implementation of IEncode
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows.Forms.VisualStyles;

    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Interop.Providers.Interfaces;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Exceptions;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Factories;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    using EncodeTask = Model.EncodeTask;
    using HandBrakeInstanceManager = Instance.HandBrakeInstanceManager;
    using IEncode = Interfaces.IEncode;
    using LogService = Logging.LogService;

    /// <summary>
    /// LibHB Implementation of IEncode
    /// </summary>
    public class LibEncode : EncodeBase, IEncode
    {
        private readonly IUserSettingService userSettingService;
        private readonly ILogInstanceManager logInstanceManager;
        private readonly IHbFunctionsProvider hbFunctionsProvider;
        private IEncodeInstance instance;
        private DateTime startTime;
        private EncodeTask currentTask;
        private HBConfiguration currentConfiguration;
        private bool isPreviewInstance;
        private bool isLoggingInitialised;

        public LibEncode(IHbFunctionsProvider hbFunctionsProvider, IUserSettingService userSettingService, ILogInstanceManager logInstanceManager) : base(userSettingService)
        {
            this.userSettingService = userSettingService;
            this.logInstanceManager = logInstanceManager;
            this.hbFunctionsProvider = hbFunctionsProvider;
        }

        public bool IsPasued { get; private set; }

        public void Start(EncodeTask task, HBConfiguration configuration, string basePresetName)
        {
            try
            {
                // Sanity Checking and Setup
                if (this.IsEncoding)
                {
                    throw new GeneralApplicationException(Resources.Queue_AlreadyEncoding, Resources.Queue_AlreadyEncodingSolution, null);
                }

                // Setup
                this.startTime = DateTime.Now;
                this.currentTask = task;
                this.currentConfiguration = configuration;

                this.InitLogging(task.IsPreviewEncode);

                // Create a new HandBrake instance
                // Setup the HandBrake Instance
                this.encodeLogService.Reset(); // Reset so we have a clean log for the start of the encode.

                if (this.instance != null)
                {
                    // Cleanup
                    try
                    {
                        this.instance.EncodeCompleted -= this.InstanceEncodeCompleted;
                        this.instance.EncodeProgress -= this.InstanceEncodeProgress;
                        this.instance.Dispose();
                        this.instance = null;
                    }
                    catch (Exception exc)
                    {
                        this.ServiceLogMessage("Failed to cleanup previous instance: " + exc);
                    }
                }

                this.ServiceLogMessage("Starting Encode ...");
                if (!string.IsNullOrEmpty(basePresetName))
                {
                    this.TimedLogMessage(string.Format("base preset: {0}", basePresetName));
                }

                int verbosity = this.userSettingService.GetUserSetting<int>(UserSettingConstants.Verbosity);
                this.instance = task.IsPreviewEncode ? HandBrakeInstanceManager.GetPreviewInstance(verbosity, this.userSettingService) : HandBrakeInstanceManager.GetEncodeInstance(verbosity, configuration, this.encodeLogService, userSettingService);
                
                this.instance.EncodeCompleted += this.InstanceEncodeCompleted;
                this.instance.EncodeProgress += this.InstanceEncodeProgress;

                this.IsEncoding = true;
                this.isPreviewInstance = task.IsPreviewEncode;

                // Verify the Destination Path Exists, and if not, create it.
                this.VerifyEncodeDestinationPath(task);

                // Get an EncodeJob object for the Interop Library
                this.instance.StartEncode(EncodeTaskFactory.Create(task, configuration, hbFunctionsProvider.GetHbFunctionsWrapper()));

                // Fire the Encode Started Event
                this.InvokeEncodeStarted(System.EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.IsEncoding = false;

                this.ServiceLogMessage("Failed to start encoding ..." + Environment.NewLine + exc);
                this.InvokeEncodeCompleted(new EventArgs.EncodeCompletedEventArgs(false, exc, "Unable to start encoding", this.currentTask.Source, this.currentTask.Destination, null, 0));
            }
        }

        public void Pause()
        {
            if (this.instance != null)
            {
                this.instance.PauseEncode();
                this.ServiceLogMessage("Encode Paused");
                this.IsPasued = true;
            }
        }

        public void Resume()
        {
            if (this.instance != null)
            {
                this.instance.ResumeEncode();
                this.ServiceLogMessage("Encode Resumed");
                this.IsPasued = false;
            }
        }

        public void Stop()
        {
            try
            {
                this.IsEncoding = false;
                if (this.instance != null)
                {
                    this.instance.StopEncode();
                    this.ServiceLogMessage("Encode Stopped");
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }
        }

        public EncodeTask GetActiveJob()
        {
            if (this.currentTask != null)
            {
                EncodeTask task = new EncodeTask(this.currentTask); // Shallow copy our current instance.
                return task;
            }

            return null;
        }

        protected void ServiceLogMessage(string message)
        {
            this.encodeLogService.LogMessage(string.Format("{0}# {1}", Environment.NewLine, message));
        }

        protected void TimedLogMessage(string message)
        {
            this.encodeLogService.LogMessage(string.Format("[{0}] {1}", DateTime.Now.ToString("hh:mm:ss"), message));
        }

        private void InstanceEncodeProgress(object sender, EncodeProgressEventArgs e)
        {
            EventArgs.EncodeProgressEventArgs args = new EventArgs.EncodeProgressEventArgs
            {
                AverageFrameRate = e.AverageFrameRate, 
                CurrentFrameRate = e.CurrentFrameRate, 
                EstimatedTimeLeft = e.EstimatedTimeLeft, 
                PercentComplete = e.FractionComplete * 100, 
                Task = e.Pass, 
                TaskCount = e.PassCount,
                ElapsedTime = DateTime.Now - this.startTime, 
                PassId = e.PassId,
                IsMuxing = e.StateCode == TaskState.Muxing.Code,
                IsSearching = e.StateCode == TaskState.Searching.Code
            };

            this.InvokeEncodeStatusChanged(args);
        }
        
        private void InstanceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.IsEncoding = false;

            string completeMessage = "Job Completed!";
            switch (e.Error)
            {
                case 0:
                    break;
                case 1:
                    completeMessage = "Job Cancelled!";
                    break;
                case 2:
                    completeMessage = string.Format("Job Failed. Check log and input settings ({0})", e.Error);
                    break;
                case 3:
                    completeMessage = string.Format("Job Failed to Initialise. Check log and input settings ({0})", e.Error);
                    break;
                default:
                    completeMessage = string.Format("Job Failed ({0})", e.Error);
                    break;
            }
            
            this.ServiceLogMessage(completeMessage);

            // Handling Log Data 
            string hbLog = this.ProcessLogs(this.currentTask.Destination, this.isPreviewInstance, this.currentConfiguration);
            long filesize = this.GetFilesize(this.currentTask.Destination);

            // Raise the Encode Completed Event.
            this.InvokeEncodeCompleted(
                e.Error != 0
                    ? new EventArgs.EncodeCompletedEventArgs(false, null, e.Error.ToString(), this.currentTask.Source, this.currentTask.Destination, hbLog, filesize)
                    : new EventArgs.EncodeCompletedEventArgs(true, null, string.Empty, this.currentTask.Source, this.currentTask.Destination, hbLog, filesize));
        }

        private long GetFilesize(string destination)
        {
            try
            {
                if (!string.IsNullOrEmpty(destination) && File.Exists(destination))
                {
                    return new FileInfo(destination).Length;
                }

                return 0;
            }
            catch (Exception e)
            {
                this.ServiceLogMessage("Unable to get final filesize ..." + Environment.NewLine + e);
                Debug.WriteLine(e);
            }

            return 0;
        }

        private void InitLogging(bool isPreview)
        {
            if (!isLoggingInitialised)
            {
                string logType = isPreview ? "preview" : "encode";
                string filename = string.Format("activity_log.{0}.{1}.txt", logType, GeneralUtilities.ProcessId);
                string logFile = Path.Combine(DirectoryUtilities.GetLogDirectory(), filename);
                this.encodeLogService = new LogService();
                this.encodeLogService.ConfigureLogging(logFile);
                this.logInstanceManager.RegisterLoggerInstance(filename, this.encodeLogService);
                isLoggingInitialised = true;
            }
        }
    }
}
