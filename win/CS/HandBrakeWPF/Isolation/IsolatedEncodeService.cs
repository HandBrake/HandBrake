// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IsolatedEncodeService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Isolated Scan Service
//   This is an implementation of the IEncode implementation that runs scans on a seperate process
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Isolation
{
    using System;
    using System.Threading;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Isolation.Interfaces;
    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// Isolated Scan Service. 
    /// This is an implementation of the IEncode implementation that runs scans on a seperate process
    /// </summary>
    public class IsolatedEncodeService : BackgroundServiceConnector, IIsolatedEncodeService
    {
        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="IsolatedEncodeService"/> class. 
        /// </summary>
        /// <param name="errorService">
        /// The error Service.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public IsolatedEncodeService(IErrorService errorService, IUserSettingService userSettingService)
            : base(errorService, userSettingService)
        {
            try
            {
                if (this.CanConnect())
                {
                    this.Connect();
                }
            }
            catch (Exception exception)
            {
                errorService.ShowError(
                    "Unable to connect to scan worker process.", "Try restarting HandBrake", exception);
            }
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
                return Service.EncodeActivityLog;
            }
        }

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding
        {
            get
            {
                return Service.IsEncoding;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// The encode completed callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        public override void EncodeCompletedCallback(EncodeCompletedEventArgs eventArgs)
        {
            if (this.EncodeCompleted != null)
            {
                ThreadPool.QueueUserWorkItem(delegate { this.EncodeCompleted(this, eventArgs); });
            }

            base.EncodeCompletedCallback(eventArgs);
        }

        /// <summary>
        /// The encode progress callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        public override void EncodeProgressCallback(EncodeProgressEventArgs eventArgs)
        {
            if (this.EncodeStatusChanged != null)
            {
                ThreadPool.QueueUserWorkItem(delegate { this.EncodeStatusChanged(this, eventArgs); });
            }

            base.EncodeProgressCallback(eventArgs);
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
            ThreadPool.QueueUserWorkItem(delegate { Service.ProcessEncodeLogs(destination); });
        }

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        public void SafelyStop()
        {
            ThreadPool.QueueUserWorkItem(delegate { Service.StopEncode(); });
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
            ThreadPool.QueueUserWorkItem(
                delegate { Service.StartEncode(job, enableLogging); });
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void Stop()
        {
            ThreadPool.QueueUserWorkItem(delegate { Service.StopEncode(); });
        }

        #endregion

        #endregion
    }
}