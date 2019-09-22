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

    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Interop.Providers.Interfaces;
    using HandBrake.Interop.Model;

    using HandBrakeWPF.Exceptions;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Encode.Factories;
    using HandBrakeWPF.Services.Interfaces;

    using EncodeTask = Model.EncodeTask;
    using HandBrakeInstanceManager = Instance.HandBrakeInstanceManager;
    using IEncode = Interfaces.IEncode;
    using ILog = Logging.Interfaces.ILog;
    using LogLevel = Logging.Model.LogLevel;
    using LogMessageType = Logging.Model.LogMessageType;
    using LogService = Logging.LogService;

    /// <summary>
    /// LibHB Implementation of IEncode
    /// </summary>
    public class LibEncode : EncodeBase, IEncode
    {
        #region Private Variables

        private ILog log = LogService.GetLogger();
        private readonly IHbFunctionsProvider hbFunctionsProvider;
        private IEncodeInstance instance;
        private DateTime startTime;
        private EncodeTask currentTask;
        private HBConfiguration currentConfiguration;
        private bool isPreviewInstance;

        #endregion

        public LibEncode(IHbFunctionsProvider hbFunctionsProvider)
        {
            this.hbFunctionsProvider = hbFunctionsProvider;
        }

        /// <summary>
        /// Gets a value indicating whether is pasued.
        /// </summary>
        public bool IsPasued { get; private set; }

        /// <summary>
        /// Start with a LibHb EncodeJob Object
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
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

                // Create a new HandBrake instance
                // Setup the HandBrake Instance
                this.log.Reset(); // Reset so we have a clean log for the start of the encode.

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

                this.instance = task.IsPreviewEncode ? HandBrakeInstanceManager.GetPreviewInstance(configuration.Verbosity, configuration) : HandBrakeInstanceManager.GetEncodeInstance(configuration.Verbosity, configuration);
                
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
                this.InvokeEncodeCompleted(new EventArgs.EncodeCompletedEventArgs(false, exc, "Unable to start encoding", task.Source, null, 0));
            }
        }

        /// <summary>
        /// Pause the currently running encode.
        /// </summary>
        public void Pause()
        {
            if (this.instance != null)
            {
                this.instance.PauseEncode();
                this.ServiceLogMessage("Encode Paused");
                this.IsPasued = true;
            }
        }

        /// <summary>
        /// Resume the currently running encode.
        /// </summary>
        public void Resume()
        {
            if (this.instance != null)
            {
                this.instance.ResumeEncode();
                this.ServiceLogMessage("Encode Resumed");
                this.IsPasued = false;
            }
        }

        /// <summary>
        /// Kill the process
        /// </summary>
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
                EncodeTask task = new EncodeTask(this.currentTask); // Decouple our current copy.
                return task;
            }

            return null;
        }

        #region HandBrakeInstance Event Handlers.

        /// <summary>
        /// Service Log Message.
        /// </summary>
        /// <param name="message">Log message content</param>
        protected void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("{0}# {1}{0}", Environment.NewLine, message), LogMessageType.ScanOrEncode, LogLevel.Info);
        }

        protected void TimedLogMessage(string message)
        {
            this.log.LogMessage(string.Format("[{0}] {1}", DateTime.Now.ToString("hh:mm:ss"), message), LogMessageType.ScanOrEncode, LogLevel.Info);
        }

        /// <summary>
        /// Encode Progress Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The Interop.EncodeProgressEventArgs.
        /// </param>
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

        /// <summary>
        /// Encode Completed Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void InstanceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.IsEncoding = false;
            this.ServiceLogMessage("Encode Completed ...");
           
            // Handling Log Data 
            string hbLog = this.ProcessLogs(this.currentTask.Destination, this.isPreviewInstance, this.currentConfiguration);
            long filesize = this.GetFilesize(this.currentTask.Destination);

            // Raise the Encode Completed Event.
            this.InvokeEncodeCompleted(
                e.Error
                    ? new EventArgs.EncodeCompletedEventArgs(false, null, string.Empty, this.currentTask.Destination, hbLog, filesize)
                    : new EventArgs.EncodeCompletedEventArgs(true, null, string.Empty, this.currentTask.Destination, hbLog, filesize));
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

        #endregion
    }
}
