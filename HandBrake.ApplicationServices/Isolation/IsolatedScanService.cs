// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IsolatedScanService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Isolated Scan Service
//   This is an implementation of the IScan implementation that runs scans on a seperate process
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Isolation
{
    using System;
    using System.Threading;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Isolated Scan Service. 
    /// This is an implementation of the IScan implementation that runs scans on a seperate process
    /// </summary>
    public class IsolatedScanService : BackgroundServiceConnector, IScan
    {
        #region Constants and Fields

        /// <summary>
        /// The post action.
        /// </summary>
        private Action<bool> postScanAction;

        #endregion

        #region Events

        /// <summary>
        /// The scan completed.
        /// </summary>
        public event ScanCompletedStatus ScanCompleted;

        /// <summary>
        /// The scan stared.
        /// </summary>
        public event EventHandler ScanStared;

        /// <summary>
        /// The scan status changed.
        /// </summary>
        public event ScanProgessStatus ScanStatusChanged;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="IsolatedScanService"/> class.
        /// </summary>
        /// <param name="port">
        /// The port.
        /// </param>
        public IsolatedScanService(string port)
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

        #region Properties

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                return this.Service.ScanActivityLog;
            }
        }

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        public bool IsScanning
        {
            get
            {
                return this.Service.IsScanning;
            }
        }

        /// <summary>
        /// Gets the Souce Data.
        /// </summary>
        public Source SouceData
        {
            get
            {
                return this.Service.SouceData;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// The scan completed callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        public override void ScanCompletedCallback(ScanCompletedEventArgs eventArgs)
        {
            if (this.postScanAction != null)
            {
                this.postScanAction(true);
            }

            if (this.ScanCompleted != null)
            {
                ThreadPool.QueueUserWorkItem(delegate { this.ScanCompleted(this, eventArgs); });  
            }

            base.ScanCompletedCallback(eventArgs);
        }

        /// <summary>
        /// The scan progress callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        public override void ScanProgressCallback(ScanProgressEventArgs eventArgs)
        {
            if (this.ScanStatusChanged != null)
            {
                ThreadPool.QueueUserWorkItem(delegate { this.ScanStatusChanged(this, eventArgs); });  
            }

            base.ScanProgressCallback(eventArgs);
        }

        /// <summary>
        /// The scan started callback.
        /// </summary>
        public override void ScanStartedCallback()
        {
            if (this.ScanStared != null)
            {
                ThreadPool.QueueUserWorkItem(delegate { this.ScanStared(this, EventArgs.Empty); });
            }

            base.ScanStartedCallback();
        }

        #endregion

        #region Implemented Interfaces

        #region IScan

        /// <summary>
        /// Take a Scan Log file, and process it as if it were from the CLI.
        /// </summary>
        /// <param name="path">
        /// The path to the log file.
        /// </param>
        public void DebugScanLog(string path)
        {
            throw new NotImplementedException("Not available in process isolation mode!");
        }

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">
        /// Path to the file to scan
        /// </param>
        /// <param name="title">
        /// int title number. 0 for scan all
        /// </param>
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        /// <param name="postAction">
        /// The post Action.
        /// </param>
        public void Scan(string sourcePath, int title, int previewCount, Action<bool> postAction)
        {
            this.postScanAction = postAction;
            this.Service.ScanSource(sourcePath, title, previewCount);
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            this.Service.StopScan();
        }

        #endregion

        #endregion
    }
}