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
    using System.Linq;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;
    using HandBrake.ApplicationServices.Services.Scan;
    using HandBrake.ApplicationServices.Services.Scan.Model;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop;
    using HandBrake.Interop.EventArgs;
    using HandBrake.Interop.Interfaces;
    using HandBrake.Interop.Model;

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

        /// <summary>
        /// A local instance of the scanned source.
        /// </summary>
        private Source scannedSource;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibEncode"/> class.
        /// </summary>
        public LibEncode()
        {
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
            this.currentTask = job;

            // Create a new HandBrake instance
            // Setup the HandBrake Instance
            HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
            this.instance = HandBrakeInstanceManager.GetEncodeInstance(job.Configuration.Verbosity);
            this.instance.EncodeCompleted += this.InstanceEncodeCompleted;
            this.instance.EncodeProgress += this.InstanceEncodeProgress;
            
            try
            {
                // Sanity Checking and Setup
                if (this.IsEncoding)
                {
                    throw new Exception("HandBrake is already encoding.");
                }

                this.IsEncoding = true;

                // Enable logging if required.
                try
                {
                    this.SetupLogging(job, true);
                }
                catch (Exception)
                {
                    this.IsEncoding = false;
                    throw;
                }

                // Verify the Destination Path Exists, and if not, create it.
                this.VerifyEncodeDestinationPath(job);

                // We have to scan the source again but only the title so the HandBrake instance is initialised correctly. 
                // Since the UI sends the crop params down, we don't have to do all the previews.

                this.instance.ScanCompleted += delegate
                {
                    // Process into internal structures.
                    this.scannedSource = new Source { Titles = LibScan.ConvertTitles(this.instance.Titles, this.instance.FeatureTitle) }; // TODO work around the bad Internal API.
                    this.ScanCompleted(job, this.instance);
                };

                HandBrakeUtils.SetDvdNav(!job.Configuration.IsDvdNavDisabled);

                ServiceLogMessage("Scanning title for encoding ... ");

                this.instance.StartScan(job.ScannedSourcePath, job.Configuration.PreviewScanCount, TimeSpan.FromSeconds(job.Configuration.MinScanDuration), job.Task.Title);
            }
            catch (Exception exc)
            {
                ServiceLogMessage("Scan Failed ... " + Environment.NewLine + exc);
                this.InvokeEncodeCompleted(new EventArgs.EncodeCompletedEventArgs(false, exc, "An Error has occured.", this.currentTask.Task.Destination));
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
            ServiceLogMessage("Scan Completed. Setting up the job for encoding ...");

            // Get an EncodeJob object for the Interop Library
            EncodeJob encodeJob = InteropModelCreator.GetEncodeJob(job);

            // Start the Encode
            Title title = this.scannedSource.Titles.FirstOrDefault(t => t.TitleNumber == job.Task.Title);
            if (title == null)
            {
                ServiceLogMessage("Title not found.");
                throw new Exception("Unable to get title for encoding. Encode Failed.");
            }

            Interop.Model.Scan.Title scannedTitle = new Interop.Model.Scan.Title
                                                        {
                                                            Resolution = new Size(title.Resolution.Width, title.Resolution.Height), 
                                                            ParVal = new Size(title.ParVal.Width, title.ParVal.Height), 
                                                            FramerateDenominator = title.FramerateDenominator, 
                                                            FramerateNumerator = title.FramerateNumerator, 
                                                        };
            
            // TODO fix this tempory hack to pass in the required title information into the factory.
            try
            {
                ServiceLogMessage("Starting Encode ...");
                instance.StartEncode(encodeJob, scannedTitle);
            }
            catch (Exception exc)
            {
                ServiceLogMessage("Failed to start encoding ..." + Environment.NewLine + exc);
                this.InvokeEncodeCompleted(new EventArgs.EncodeCompletedEventArgs(false, exc, "Unable to start encoding", job.Task.Source));
            }

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
