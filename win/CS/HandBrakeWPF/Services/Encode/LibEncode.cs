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
    using System.Globalization;
    using System.IO;

    using HandBrake.App.Core.Exceptions;
    using HandBrake.App.Core.Utilities;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.State;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Factories;
    using HandBrakeWPF.Services.Encode.Interfaces;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;

    using EncodeTask = Model.EncodeTask;
    using HandBrakeInstanceManager = Instance.HandBrakeInstanceManager;
    using IEncode = Interfaces.IEncode;
    using LogService = Logging.LogService;

    public class LibEncode : IEncode
    {
        private readonly IUserSettingService userSettingService;
        private readonly ILogInstanceManager logInstanceManager;
        private readonly IPortService portService;
        private readonly object portLock = new object();
        private readonly EncodeTaskFactory encodeTaskFactory;
        private ILog encodeLogService;

        private IEncodeInstance instance;
        private DateTime startTime;
        private EncodeTask currentTask;
        private bool isPreviewInstance;
        private bool isLoggingInitialised;
        private bool isEncodeComplete;
        private int encodeCounter;
        
        public LibEncode(IUserSettingService userSettingService, ILogInstanceManager logInstanceManager, int encodeCounter, IPortService portService)
        {
            this.userSettingService = userSettingService;
            this.logInstanceManager = logInstanceManager;
            this.encodeCounter = encodeCounter;
            this.portService = portService;
            this.encodeTaskFactory = new EncodeTaskFactory(this.userSettingService, true);
        }

        public event EventHandler EncodeStarted;

        public event EncodeCompletedStatus EncodeCompleted;

        public event EncodeProgressStatus EncodeStatusChanged;

        public bool IsPaused { get; private set; }

        public bool IsEncoding { get; protected set; }

        public void Start(EncodeTask task, string basePresetName)
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
                this.isPreviewInstance = task.IsPreviewEncode;

                if (task.IsPreviewEncode)
                {
                    this.encodeLogService = this.logInstanceManager.ApplicationLogInstance;
                    this.encodeLogService.Reset();
                }
                else if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled) && Portable.IsProcessIsolationEnabled())
                {
                    this.InitRemoteLogging(task.Destination);
                }
                else
                {
                    this.encodeLogService = this.logInstanceManager.ApplicationLogInstance;
                    this.encodeLogService.Reset();
                }
                
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

                // Prevent port stealing if multiple jobs start at the same time.
                lock (this.portLock) 
                {
                    this.instance = HandBrakeInstanceManager.GetEncodeInstance(verbosity, this.encodeLogService, this.userSettingService, this.portService);

                    this.instance.EncodeCompleted += this.InstanceEncodeCompleted;
                    this.instance.EncodeProgress += this.InstanceEncodeProgress;

                    this.IsEncoding = true;

                    // Verify the Destination Path Exists, and if not, create it.
                    this.VerifyEncodeDestinationPath(task);

                    // Get an EncodeJob object for the Interop Library
                    JsonEncodeObject work = this.encodeTaskFactory.Create(task);

                    this.instance.StartEncode(work);
                }

                // Fire the Encode Started Event
                this.InvokeEncodeStarted(System.EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.IsEncoding = false;

                this.ServiceLogMessage("Failed to start encoding ..." + Environment.NewLine + exc);
                this.InvokeEncodeCompleted(new EventArgs.EncodeCompletedEventArgs(false, exc, "Unable to start encoding", this.currentTask.Source, this.currentTask.Destination, null, 0, 3));
            }
        }

        public void Pause()
        {
            if (this.instance != null)
            {
                this.instance.PauseEncode();
                this.ServiceLogMessage("Encode Paused");
                this.IsPaused = true;
            }
        }

        public void Resume()
        {
            if (this.instance != null)
            {
                this.instance.ResumeEncode();
                this.ServiceLogMessage("Encode Resumed");
                this.IsPaused = false;
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
            this.encodeLogService.LogMessage(string.Format("{1} # {0}{1}", message, Environment.NewLine));
        }

        protected void TimedLogMessage(string message)
        {
            this.encodeLogService.LogMessage(string.Format("[{0}] {1}", DateTime.Now.ToString("HH:mm:ss"), message));
        }

        private void InvokeEncodeStatusChanged(EventArgs.EncodeProgressEventArgs e)
        {
            EncodeProgressStatus handler = this.EncodeStatusChanged;
            handler?.Invoke(this, e);
        }

        private void InvokeEncodeCompleted(EventArgs.EncodeCompletedEventArgs e)
        {
            EncodeCompletedStatus handler = this.EncodeCompleted;
            handler?.Invoke(this, e);
        }

        private void InvokeEncodeStarted(System.EventArgs e)
        {
            EventHandler handler = this.EncodeStarted;
            handler?.Invoke(this, e);
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

            if (this.isEncodeComplete)
            {
                return; // Prevent phantom events bubbling up the stack. 
            }

            this.isEncodeComplete = true;

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
            string hbLog = this.ProcessLogs(this.currentTask.Destination);
            long filesize = this.GetFilesize(this.currentTask.Destination);

            // Raise the Encode Completed Event.
            this.InvokeEncodeCompleted(
                e.Error != 0
                    ? new EventArgs.EncodeCompletedEventArgs(false, null, e.Error.ToString(), this.currentTask.Source, this.currentTask.Destination, hbLog, filesize, e.Error)
                    : new EventArgs.EncodeCompletedEventArgs(true, null, string.Empty, this.currentTask.Source, this.currentTask.Destination, hbLog, filesize, e.Error));

            this.logInstanceManager.Deregister(Path.GetFileName(hbLog));
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

        private void InitRemoteLogging(string destination)
        {
            if (!this.isLoggingInitialised)
            {
                string logType = this.isPreviewInstance ? "preview" : "encode";
                string destinationFile = Path.GetFileNameWithoutExtension(destination);
                string logFileName = string.Format("{0}_{1}_{2}.txt", destinationFile, logType, DateTime.Now.ToString(CultureInfo.InvariantCulture).Replace("/", ".").Replace(":", "-"));
                string fullLogPath = Path.Combine(DirectoryUtilities.GetLogDirectory(), logFileName);

                this.encodeLogService = new LogService();
                this.encodeLogService.ConfigureLogging(logFileName, fullLogPath);
                this.encodeLogService.SetId(this.encodeCounter);
                this.logInstanceManager.Register(logFileName, this.encodeLogService, false);
                this.isLoggingInitialised = true;
            }
        }

        private string ProcessLogs(string destination)
        {
            try
            {
                string logDir = DirectoryUtilities.GetLogDirectory();
                string filename = this.encodeLogService.FileName;
                string logContent = this.encodeLogService.GetFullLog();

                // Make sure the log directory exists.
                if (!Directory.Exists(logDir))
                {
                    Directory.CreateDirectory(logDir);
                }

                // Copy the Log to HandBrakes log folder in the users application data folder.
                // Only needed for process isolation mode. Worker will handle it's own logging.
                bool processIsolationEnabled = this.userSettingService.GetUserSetting<bool>(UserSettingConstants.ProcessIsolationEnabled) && Portable.IsProcessIsolationEnabled();

                if (!processIsolationEnabled)
                {
                    string logType = this.isPreviewInstance ? "preview" : "encode";
                    string destinationFile = Path.GetFileNameWithoutExtension(destination);
                    string logFileName = string.Format("{0}_{1}_{2}.txt", destinationFile, logType, DateTime.Now.ToString(CultureInfo.InvariantCulture).Replace("/", ".").Replace(":", "-"));
                    this.WriteFile(logContent, Path.Combine(logDir, logFileName));
                    filename = logFileName;
                }

                // Save a copy of the log file in the same location as the encode.
                if (this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogWithVideo))
                {
                    this.WriteFile(logContent, Path.Combine(Path.GetDirectoryName(destination), filename));
                }

                // Save a copy of the log file to a user specified location
                if (Directory.Exists(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory)) && this.userSettingService.GetUserSetting<bool>(UserSettingConstants.SaveLogToCopyDirectory))
                {
                    this.WriteFile(logContent, Path.Combine(this.userSettingService.GetUserSetting<string>(UserSettingConstants.SaveLogCopyDirectory), filename));
                }

                return Path.Combine(logDir, filename);
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc); // This exception doesn't warrant user interaction, but it should be logged
            }

            return null;
        }

        private void VerifyEncodeDestinationPath(EncodeTask task)
        {
            // Make sure the path exists, attempt to create it if it doesn't
            try
            {
                string path = Directory.GetParent(task.Destination).ToString();
                if (!Directory.Exists(path))
                {
                    Directory.CreateDirectory(path);
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    "Unable to create directory for the encoded output.", "Please verify that you have a valid path.", exc);
            }
        }

        private void WriteFile(string content, string fileName)
        {
            try
            {
                using (StreamWriter fileWriter = new StreamWriter(fileName) { AutoFlush = true })
                {
                    fileWriter.Write(content);
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }
        }
    }
}
