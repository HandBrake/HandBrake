// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IsolatedEncodeService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Isolated Scan Service
//   This is an implementation of the IEncode implementation that runs scans on a seperate process
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Isolation
{
    using System;
    using System.Threading;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Isolated Scan Service. 
    /// This is an implementation of the IEncode implementation that runs scans on a seperate process
    /// </summary>
    public class IsolatedEncodeService : BackgroundServiceConnector, IEncode
    {
        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="IsolatedEncodeService"/> class. 
        /// </summary>
        /// <param name="port">
        /// The port.
        /// </param>
        public IsolatedEncodeService(string port)
        {
            try
            {
                if (this.CanConnect())
                {
                    this.Connect(port);
                }
            }
            catch (Exception exception)
            {
                throw new GeneralApplicationException("Unable to connect to scan worker process.", "Try restarting HandBrake", exception);
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
                return this.IsConnected ? this.Service.EncodeActivityLog : "Unable to connect to background worker service ...";
            }
        }

        /// <summary>
        /// Gets the log index.
        /// </summary>
        public int LogIndex
        {
            get
            {
                return -1;
            }
        }

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        public bool IsEncoding
        {
            get
            {
                return this.IsConnected && this.Service.IsEncoding;
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
            ThreadPool.QueueUserWorkItem(delegate { this.Service.ProcessEncodeLogs(destination); });
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
                delegate { this.Service.StartEncode(job, enableLogging); });
        }

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        public void Stop()
        {
            ThreadPool.QueueUserWorkItem(delegate { this.Service.StopEncode(); });
        }

        #endregion

        #endregion
    }
}