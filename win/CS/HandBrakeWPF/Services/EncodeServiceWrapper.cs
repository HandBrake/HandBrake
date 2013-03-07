// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeServiceWrapper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   We have multiple implementations of IEncode. This is a wrapper class for the GUI so that the 
//   implementation used is controllable via user settings.
//   Over time, this class will go away when the LibHB and process isolation code matures.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;

    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Isolation;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop;

    using EncodeCompletedEventArgs = HandBrake.ApplicationServices.EventArgs.EncodeCompletedEventArgs;
    using EncodeProgressEventArgs = HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs;

    /// <summary>
    /// We have multiple implementations of Iencode. This is a wrapper class for the GUI so that the 
    /// implementation used is controllable via user settings.
    /// Over time, this class will go away when the LibHB and process isolation code matures.
    /// </summary>
    public class EncodeServiceWrapper : IEncodeServiceWrapper
    {
        #region Constants and Fields

        /// <summary>
        /// The encode service.
        /// </summary>
        private readonly IEncode encodeService;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="EncodeServiceWrapper"/> class.
        /// </summary>
        /// <param name="userSettingService">
        /// The user setting service.
        /// </param>
        public EncodeServiceWrapper(IUserSettingService userSettingService)
        {
            var useLibHb = userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableLibHb);
            var useProcessIsolation =
                userSettingService.GetUserSetting<bool>(UserSettingConstants.EnableProcessIsolation);
            var port = userSettingService.GetUserSetting<string>(UserSettingConstants.ServerPort);

            if (useLibHb)
            {
                try
                {
                    if (useProcessIsolation)
                    {
                        this.encodeService = new IsolatedEncodeService(port);
                    }
                    else
                    {
                        if (ScanServiceWrapper.HandbrakeInstance == null)
                        {
                            ScanServiceWrapper.HandbrakeInstance = new HandBrakeInstance();
                        }

                        this.encodeService = new LibEncode(userSettingService, ScanServiceWrapper.HandbrakeInstance);
                    }
                }
                catch (Exception exc)
                {
                    // Try to recover from errors.
                    userSettingService.SetUserSetting(UserSettingConstants.EnableLibHb, false);
                    throw new GeneralApplicationException(
                        "Unable to initialise LibHB or Background worker service",
                        "Falling back to using HandBrakeCLI.exe. Setting has been reset",
                        exc);
                }
            }
            else
            {
                this.encodeService = new Encode(userSettingService);
            }

            this.encodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;
            this.encodeService.EncodeStarted += this.EncodeServiceEncodeStarted;
            this.encodeService.EncodeStatusChanged += this.EncodeServiceEncodeStatusChanged;
        }

        #endregion

        #region Events

        /// <summary>
        /// The encode completed.
        /// </summary>
        public event EncodeCompletedStatus EncodeCompleted;

        /// <summary>
        /// The encode started.
        /// </summary>
        public event EventHandler EncodeStarted;

        /// <summary>
        /// The encode status changed.
        /// </summary>
        public event EncodeProgessStatus EncodeStatusChanged;

        #endregion

        #region Properties

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                return this.encodeService.ActivityLog;
            }
        }

        /// <summary>
        /// Gets the log index.
        /// </summary>
        public int LogIndex
        {
            get
            {
                return this.encodeService.LogIndex;
            }
        }

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding
        {
            get
            {
                return this.encodeService.IsEncoding;
            }
        }

        #endregion

        #region Implemented Interfaces

        #region IEncode

        /// <summary>
        /// Copy the log file to the desired destinations
        /// </summary>
        /// <param name="destination">
        /// The destination.
        /// </param>
        public void ProcessLogs(string destination)
        {
            this.encodeService.ProcessLogs(destination);
        }

        /// <summary>
        /// Shutdown the service.
        /// </summary>
        public void Shutdown()
        {
            this.encodeService.Shutdown();
            this.encodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
            this.encodeService.EncodeStarted -= this.EncodeServiceEncodeStarted;
            this.encodeService.EncodeStatusChanged -= this.EncodeServiceEncodeStatusChanged;
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
            this.encodeService.Start(job, enableLogging);
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void Stop()
        {
            this.encodeService.Stop();
        }

        #endregion

        #endregion

        #region Methods

        /// <summary>
        /// The encode service_ encode completed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeCompletedEventArgs.
        /// </param>
        private void EncodeServiceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            if (EncodeCompleted != null)
            {
                this.EncodeCompleted(sender, e);
            }
        }

        /// <summary>
        /// The encode service_ encode started.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs
        /// </param>
        private void EncodeServiceEncodeStarted(object sender, EventArgs e)
        {
            if (EncodeStarted != null)
            {
                this.EncodeStarted(sender, e);
            }
        }

        /// <summary>
        /// The encode service_ encode status changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EncodeProgressEventArgs.
        /// </param>
        private void EncodeServiceEncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            if (EncodeStatusChanged != null)
            {
                this.EncodeStatusChanged(sender, e);
            }
        }

        #endregion
    }
}