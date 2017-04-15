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

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.EventArgs;
    using HandBrake.ApplicationServices.Interop.Interfaces;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Logging;
    using HandBrake.ApplicationServices.Services.Logging.Interfaces;
    using HandBrake.ApplicationServices.Services.Logging.Model;

    using HandBrakeWPF.Exceptions;
    using HandBrakeWPF.Services.Encode.Factories;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;
    using IEncode = HandBrakeWPF.Services.Encode.Interfaces.IEncode;

    /// <summary>
    /// LibHB Implementation of IEncode
    /// </summary>
    public class LibEncode : HandBrakeWPF.Services.Encode.EncodeBase, IEncode
    {
        #region Private Variables

        private ILog log = LogService.GetLogger();
        private IHandBrakeInstance instance;
        private DateTime startTime;
        private EncodeTask currentTask;
        private HBConfiguration currentConfiguration;
        private bool isPreviewInstance;

        #endregion

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
        public void Start(EncodeTask task, HBConfiguration configuration)
        {
            try
            {
                // Sanity Checking and Setup
                if (this.IsEncoding)
                {
                    throw new GeneralApplicationException("HandBrake is already encoding a file.", "Please stop the current encode. If the problem persists, please restart HandBrake.", null);
                }

                // Setup
                this.startTime = DateTime.Now;
                this.currentTask = task;
                this.currentConfiguration = configuration;

                // Create a new HandBrake instance
                // Setup the HandBrake Instance
                this.log.Reset(); // Reset so we have a clean log for the start of the encode.
                this.ServiceLogMessage("Starting Encode ...");

                HandBrakeUtils.SetDvdNav(!configuration.IsDvdNavDisabled);
                this.instance = task.IsPreviewEncode ? HandBrakeInstanceManager.GetPreviewInstance(configuration.Verbosity) : HandBrakeInstanceManager.GetEncodeInstance(configuration.Verbosity);
                
                this.instance.EncodeCompleted += this.InstanceEncodeCompleted;
                this.instance.EncodeProgress += this.InstanceEncodeProgress;

                this.IsEncoding = true;
                this.isPreviewInstance = task.IsPreviewEncode;

                // Verify the Destination Path Exists, and if not, create it.
                this.VerifyEncodeDestinationPath(task);

                // Get an EncodeJob object for the Interop Library
                this.instance.StartEncode(EncodeFactory.Create(task, configuration));

                // Fire the Encode Started Event
                this.InvokeEncodeStarted(System.EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.IsEncoding = false;

                this.ServiceLogMessage("Failed to start encoding ..." + Environment.NewLine + exc);
                this.InvokeEncodeCompleted(new EventArgs.EncodeCompletedEventArgs(false, exc, "Unable to start encoding", task.Source));
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

        #region HandBrakeInstance Event Handlers.

        /// <summary>
        /// Service Log Message.
        /// </summary>
        /// <param name="message">Log message content</param>
        protected void ServiceLogMessage(string message)
        {
            this.log.LogMessage(string.Format("{0}# {1}{0}", Environment.NewLine, message), LogMessageType.ScanOrEncode, LogLevel.Info);
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
                IsMuxing = e.IsMuxing,
                IsSearching = e.IsSearching
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
            this.ProcessLogs(this.currentTask.Destination, this.isPreviewInstance, this.currentConfiguration);

            // Raise the Encode Completed EVent.
            this.InvokeEncodeCompleted(
                e.Error
                    ? new EventArgs.EncodeCompletedEventArgs(false, null, string.Empty, this.currentTask.Destination)
                    : new EventArgs.EncodeCompletedEventArgs(true, null, string.Empty, this.currentTask.Destination));
        }
        #endregion
    }
}
