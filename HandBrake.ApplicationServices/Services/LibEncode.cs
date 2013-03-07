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
    using System.Globalization;

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
        private static readonly object logLock = new object();

        /// <summary>
        /// The User Setting Service
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// The Start time of the current Encode;
        /// </summary>
        private DateTime startTime;

        /// <summary>
        /// An Instance of the HandBrake Interop Library
        /// </summary>
        private IHandBrakeInstance instance;

        /// <summary>
        /// A flag to indicate if logging is enabled or not.
        /// </summary>
        private bool loggingEnabled;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibEncode"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        /// <param name="handBrakeInstance">
        /// The hand Brake Instance.
        /// </param>
        public LibEncode(IUserSettingService userSettingService, IHandBrakeInstance handBrakeInstance)
            : base(userSettingService)
        {
            this.userSettingService = userSettingService;

            // Setup the HandBrake Instance
            this.instance = handBrakeInstance;
            this.instance.EncodeCompleted += this.InstanceEncodeCompleted;
            this.instance.EncodeProgress += this.InstanceEncodeProgress;

            HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
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
            this.startTime = DateTime.Now;
            this.loggingEnabled = enableLogging;

            try
            {
                // Sanity Checking and Setup
                if (this.IsEncoding)
                {
                    throw new Exception("HandBrake is already encoding.");
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
                if (this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.PreventSleep) )
                {
                    Win32.PreventSleep();
                }

                // Verify the Destination Path Exists, and if not, create it.
                this.VerifyEncodeDestinationPath(job);

                // Start the Encode
                this.instance.StartEncode(encodeJob);

                // Set the Process Priority
                switch (this.userSettingService.GetUserSetting<string>(ASUserSettingConstants.ProcessPriority))
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
                this.InvokeEncodeStarted(EventArgs.Empty);
            }
            catch (Exception exc)
            {
                this.InvokeEncodeCompleted(new EncodeCompletedEventArgs(false, exc, "An Error has occured."));
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
                lock (logLock)
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
                    ? new EncodeCompletedEventArgs(false, null, string.Empty)
                    : new EncodeCompletedEventArgs(true, null, string.Empty));

            if (this.userSettingService.GetUserSetting<bool>(ASUserSettingConstants.PreventSleep))
            {
                Win32.AllowSleep();
            }

            this.ShutdownFileWriter();
        }
        #endregion
    }
}
