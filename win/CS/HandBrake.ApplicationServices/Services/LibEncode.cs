// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LibEncode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   LibHB Implementation of IEncode
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Base;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop;
    using HandBrake.Interop.EventArgs;
    using HandBrake.Interop.Interfaces;
    using HandBrake.Interop.Model;

    using EncodeCompletedEventArgs = HandBrake.ApplicationServices.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs;

    /// <summary>
    /// LibHB Implementation of IEncode
    /// </summary>
    public class LibEncode : EncodeBase, IEncode
    {
        #region Private Variables

        /// <summary>
        /// Lock for the log file
        /// </summary>
        private static readonly object LogLock = new object();

        /// <summary>
        /// The instance.
        /// </summary>
        private IHandBrakeInstance instance;

        /// <summary>
        /// The Start time of the current Encode;
        /// </summary>
        private DateTime startTime;

        /// <summary>
        /// A flag to indicate if logging is enabled or not.
        /// </summary>
        private bool loggingEnabled;

        /// <summary>
        /// The Current Task
        /// </summary>
        private QueueTask currentTask;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibEncode"/> class.
        /// </summary>
        public LibEncode()
        {
            HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
        }

        /// <summary>
        /// Gets a value indicating whether can pause.
        /// </summary>
        public bool CanPause
        {
            get
            {
                return true;
            }
        }

        /// <summary>
        /// Gets a value indicating whether is pasued.
        /// </summary>
        public bool IsPasued { get; private set; }

        /// <summary>
        /// Start with a LibHb EncodeJob Object
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        public void Start(QueueTask job)
        {
            // Setup
            this.startTime = DateTime.Now;
            this.loggingEnabled = job.Configuration.IsLoggingEnabled;
            this.currentTask = job;

            // Create a new HandBrake instance
            // Setup the HandBrake Instance
            instance = new HandBrakeInstance();
            instance.Initialize(1);
            instance.EncodeCompleted += this.InstanceEncodeCompleted;
            instance.EncodeProgress += this.InstanceEncodeProgress;
            
            try
            {
                // Sanity Checking and Setup
                if (this.IsEncoding)
                {
                    throw new Exception("HandBrake is already encoding.");
                }

                this.IsEncoding = true;

                // Enable logging if required.
                if (job.Configuration.IsLoggingEnabled)
                {
                    try
                    {
                        this.SetupLogging(job);
                    }
                    catch (Exception)
                    {
                        this.IsEncoding = false;
                        throw;
                    }
                }

                // Verify the Destination Path Exists, and if not, create it.
                this.VerifyEncodeDestinationPath(job);

                // We have to scan the source again but only the title so the HandBrake instance is initialised correctly. 
                // Since the UI sends the crop params down, we don't have to do all the previews.
                instance.StartScan(job.Task.Source, job.Configuration.PreviewScanCount, job.Task.Title);

                instance.ScanCompleted += delegate
                    {
                        ScanCompleted(job, instance);
                    };
            }
            catch (Exception exc)
            {
                this.InvokeEncodeCompleted(new EncodeCompletedEventArgs(false, exc, "An Error has occured.", this.currentTask.Task.Destination));
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
                this.IsPasued = false;
            }
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public override void Stop()
        {
            try
            {
                this.IsEncoding = false;
                this.instance.StopEncode();
            }
            catch (Exception)
            {
                // Do Nothing.
            }
        }

        /// <summary>
        /// Shutdown the service.
        /// </summary>
        public void Shutdown()
        {
            // Nothing to do for this implementation.
        }

        /// <summary>
        /// The scan completed.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="instance">
        /// The instance.
        /// </param>
        private void ScanCompleted(QueueTask job, IHandBrakeInstance instance)
        {
            // Get an EncodeJob object for the Interop Library
            EncodeJob encodeJob = InteropModelCreator.GetEncodeJob(job);

            // Start the Encode
            instance.StartEncode(encodeJob, job.Configuration.PreviewScanCount);

            // Fire the Encode Started Event
            this.InvokeEncodeStarted(EventArgs.Empty);

            // Set the Process Priority
            switch (job.Configuration.ProcessPriority)
            {
                case "Realtime":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.RealTime;
                    break;
                case "High":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High;
                    break;
                case "Above Normal":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.AboveNormal;
                    break;
                case "Normal":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Normal;
                    break;
                case "Low":
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.Idle;
                    break;
                default:
                    Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.BelowNormal;
                    break;
            }
        }

        #region HandBrakeInstance Event Handlers.
        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MessageLoggedEventArgs.
        /// </param>
        private void HandBrakeInstanceErrorLogged(object sender, MessageLoggedEventArgs e)
        {
            if (this.loggingEnabled)
            {
                lock (LogLock)
                {
                    this.ProcessLogMessage(e.Message);
                }
            }
        }

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MessageLoggedEventArgs.
        /// </param>
        private void HandBrakeInstanceMessageLogged(object sender, MessageLoggedEventArgs e)
        {
            if (this.loggingEnabled)
            {
                lock (LogLock)
                {
                    this.ProcessLogMessage(e.Message);
                }
            }
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
        private void InstanceEncodeProgress(object sender, Interop.EventArgs.EncodeProgressEventArgs e)
        {
           EncodeProgressEventArgs args = new EncodeProgressEventArgs
            {
                AverageFrameRate = e.AverageFrameRate,
                CurrentFrameRate = e.CurrentFrameRate,
                EstimatedTimeLeft = e.EstimatedTimeLeft,
                PercentComplete = e.FractionComplete * 100,
                Task = e.Pass,
                ElapsedTime = DateTime.Now - this.startTime,
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
        private void InstanceEncodeCompleted(object sender, Interop.EventArgs.EncodeCompletedEventArgs e)
        {
            this.IsEncoding = false;

            this.InvokeEncodeCompleted(
                e.Error
                    ? new EncodeCompletedEventArgs(false, null, string.Empty, this.currentTask.Task.Destination)
                    : new EncodeCompletedEventArgs(true, null, string.Empty, this.currentTask.Task.Destination));

            this.ShutdownFileWriter();
        }
        #endregion
    }
}
