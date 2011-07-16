/*  LibEncode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.Text;
    using System.Threading;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Base;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop;
    using HandBrake.Interop.Model;

    using EncodeCompletedEventArgs = HandBrake.ApplicationServices.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs;

    /// <summary>
    /// TODO: Update summary.
    /// </summary>
    public class LibEncode : EncodeBase, IEncode
    {
        #region Private Variables

        /// <summary>
        /// Lock for the log file
        /// </summary>
        private static readonly object logLock = new object();

        /// <summary>
        /// The Start time of the current Encode;
        /// </summary>
        private DateTime startTime;

        /// <summary>
        /// An Instance of the HandBrake Interop Library
        /// </summary>
        private HandBrakeInstance instance;

        /// <summary>
        /// A flag to indicate if logging is enabled or not.
        /// </summary>
        private bool loggingEnabled;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibEncode"/> class.
        /// </summary>
        public LibEncode()
        {
            // Setup the HandBrake Instance
            this.instance = new HandBrakeInstance();
            this.instance.EncodeCompleted += this.InstanceEncodeCompleted;
            this.instance.EncodeProgress += this.InstanceEncodeProgress;

            HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;

            GrowlCommunicator.Register();
        }

        /// <summary>
        /// Start with a LibHb EncodeJob Object
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="enableLogging">
        /// The enable Logging.
        /// </param>
        public void Start(QueueTask job, bool enableLogging)
        {
            throw new NotImplementedException("This Method has not been completed yet");

            this.startTime = DateTime.Now;
            this.loggingEnabled = enableLogging;

            try
            {
                // Sanity Checking and Setup
                if (this.IsEncoding)
                {
                    throw new Exception("HandBrake is already encodeing.");
                }

                this.IsEncoding = true;

                // Get an EncodeJob object for the Interop Library
                EncodeJob encodeJob = InteropModelCreator.GetEncodeJob(job);

                // Enable logging if required.
                if (enableLogging)
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

                // Prvent the system from sleeping if the user asks
                if (Properties.Settings.Default.PreventSleep)
                {
                    Win32.PreventSleep();
                }

                // Verify the Destination Path Exists, and if not, create it.
                this.VerifyEncodeDestinationPath(job);

                // Start the Encode
                this.instance.StartEncode(encodeJob);

                // Set the Process Priority
                switch (Properties.Settings.Default.ProcessPriority)
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

                // Fire the Encode Started Event
                this.Invoke_encodeStarted(EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.Invoke_encodeCompleted(new EncodeCompletedEventArgs(false, exc, "An Error has occured in EncodeService.Run()"));
            }
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void Stop()
        {
            this.Stop(null);
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        /// <param name="exc">
        /// The Exception that has occured.
        /// This will get bubbled up through the EncodeCompletedEventArgs
        /// </param>
        public override void Stop(Exception exc)
        {
            this.instance.StopEncode();

            this.Invoke_encodeCompleted(
                exc == null
                    ? new EncodeCompletedEventArgs(true, null, string.Empty)
                    : new EncodeCompletedEventArgs(false, exc, "An Error has occured."));
        }

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        public void SafelyStop()
        {
            throw new NotImplementedException("This Method is not used in the LibEncode service. You should use the Stop() method instead! ");
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
                lock (logLock)
                {
                    this.LogBuffer.AppendLine(e.Message);
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
                lock (logLock)
                {
                    this.LogBuffer.AppendLine(e.Message);
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
        private void InstanceEncodeProgress(object sender, Interop.EncodeProgressEventArgs e)
        {
           EncodeProgressEventArgs args = new EncodeProgressEventArgs
            {
                AverageFrameRate = e.AverageFrameRate,
                CurrentFrameRate = e.CurrentFrameRate,
                EstimatedTimeLeft = e.EstimatedTimeLeft,
                PercentComplete = e.FractionComplete,
                Task = e.Pass,
                ElapsedTime = DateTime.Now - this.startTime,
            };

            this.Invoke_encodeStatusChanged(args);

            if (this.WindowsSeven.IsWindowsSeven)
            {
                int percent;
                int.TryParse(Math.Round(e.FractionComplete).ToString(), out percent);

                this.WindowsSeven.SetTaskBarProgress(percent);
            }
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
        private void InstanceEncodeCompleted(object sender, Interop.EncodeCompletedEventArgs e)
        {
            this.IsEncoding = false;

            this.Invoke_encodeCompleted(new EncodeCompletedEventArgs(e.Error, null, string.Empty));

            if (this.WindowsSeven.IsWindowsSeven)
            {
                this.WindowsSeven.SetTaskBarProgressToNoProgress();
            }

            if (Properties.Settings.Default.PreventSleep)
            {
                Win32.AllowSleep();
            }
        }
        #endregion
    }
}
