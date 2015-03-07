// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LibEncode.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   LibHB Implementation of IEncode
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Encode
{
    using System;
    using System.Diagnostics;

    using HandBrake.ApplicationServices.Interop;
    using HandBrake.ApplicationServices.Interop.EventArgs;
    using HandBrake.ApplicationServices.Interop.Interfaces;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Factories;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;

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
        /// The Current Task
        /// </summary>
        private QueueTask currentTask;

        #endregion

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
            try
            {
                // Setup
                this.startTime = DateTime.Now;
                this.currentTask = job;

                // Create a new HandBrake instance
                // Setup the HandBrake Instance
                HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
                HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
                this.instance = HandBrakeInstanceManager.GetEncodeInstance(job.Configuration.Verbosity);
                this.instance.EncodeCompleted += this.InstanceEncodeCompleted;
                this.instance.EncodeProgress += this.InstanceEncodeProgress;

                // Sanity Checking and Setup
                if (this.IsEncoding)
                {
                    throw new Exception("HandBrake is already encoding.");
                }
     
                this.IsEncoding = true;
                this.SetupLogging(job, true);

                // Verify the Destination Path Exists, and if not, create it.
                this.VerifyEncodeDestinationPath(job);

                ServiceLogMessage("Starting Encode ...");

                // Get an EncodeJob object for the Interop Library
                instance.StartEncode(EncodeFactory.Create(job.Task, job.Configuration));

                // Fire the Encode Started Event
                this.InvokeEncodeStarted(System.EventArgs.Empty);

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
            catch (Exception exc)
            {
                this.IsEncoding = false;

                ServiceLogMessage("Failed to start encoding ..." + Environment.NewLine + exc);
                this.InvokeEncodeCompleted(new EventArgs.EncodeCompletedEventArgs(false, exc, "Unable to start encoding", job.Task.Source));
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
                ServiceLogMessage("Encode Paused");
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
                ServiceLogMessage("Encode Resumed");
                this.IsPasued = false;
            }
        }

        /// <summary>
        /// Kill the process
        /// </summary>
        public override void Stop()
        {
            try
            {
                this.IsEncoding = false;
                if (instance != null)
                {
                    this.instance.StopEncode();
                    ServiceLogMessage("Encode Stopped");
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
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
            lock (LogLock)
            {
                this.ProcessLogMessage(e.Message);
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
            lock (LogLock)
            {
                this.ProcessLogMessage(e.Message);
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
        private void InstanceEncodeProgress(object sender, EncodeProgressEventArgs e)
        {
           EventArgs.EncodeProgressEventArgs args = new EventArgs.EncodeProgressEventArgs
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
        private void InstanceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            this.IsEncoding = false;
            ServiceLogMessage("Encode Completed ...");

            // Stop Logging.
            HandBrakeUtils.MessageLogged -= this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged -= this.HandBrakeInstanceErrorLogged;
            
            // Handling Log Data 
            this.ProcessLogs(this.currentTask.Task.Destination, this.currentTask.Configuration);

            // Cleanup
            this.ShutdownFileWriter();

            // Raise the Encode Completed EVent.
            this.InvokeEncodeCompleted(
                e.Error
                    ? new EventArgs.EncodeCompletedEventArgs(false, null, string.Empty, this.currentTask.Task.Destination)
                    : new EventArgs.EncodeCompletedEventArgs(true, null, string.Empty, this.currentTask.Task.Destination));
        }
        #endregion
    }
}
