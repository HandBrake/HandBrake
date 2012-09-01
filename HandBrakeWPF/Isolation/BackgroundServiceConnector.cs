// --------------------------------------------------------------------------------------------------------------------
// <copyright file="BackgroundServiceConnector.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Background Service Connector.
//   HandBrake has the ability to connect to a service app that will control HandBrakeCLI or Libhb.
//   This acts as process isolation.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Isolation
{
    using System;
    using System.Diagnostics;
    using System.ServiceModel;
    using System.Threading;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Services.Interfaces;

    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// Background Service Connector.
    /// HandBrake has the ability to connect to a service app that will control HandBrakeCLI or Libhb. 
    /// This acts as process isolation.
    /// </summary>
    public class BackgroundServiceConnector : IHbServiceCallback, IDisposable
    {
        #region Constants and Fields

        /// <summary>
        /// The error service.
        /// </summary>
        private readonly IErrorService errorService;

        /// <summary>
        /// The user setting service.
        /// </summary>
        private readonly IUserSettingService userSettingService;

        /// <summary>
        /// Gets or sets the pipe factory.
        /// DuplexChannelFactory is necessary for Callbacks.
        /// </summary>
        private static DuplexChannelFactory<IServerService> pipeFactory;

        /// <summary>
        /// The background process.
        /// </summary>
        private static Process backgroundProcess;

        #endregion

        #region Properties

        /// <summary>
        /// Initializes a new instance of the <see cref="BackgroundServiceConnector"/> class.
        /// </summary>
        /// <param name="errorService">
        /// The error service.
        /// </param>
        /// <param name="userSettingService">
        /// The user Setting Service.
        /// </param>
        public BackgroundServiceConnector(IErrorService errorService, IUserSettingService userSettingService)
        {
            this.errorService = errorService;
            this.userSettingService = userSettingService;
        }

        /// <summary>
        /// Gets or sets a value indicating whether is connected.
        /// </summary>
        public bool IsConnected { get; set; }

        /// <summary>
        /// Gets or sets the service.
        /// </summary>
        public IServerService Service { get; set; }

        #endregion

        #region Public Server Management Methods

        /// <summary>
        /// The can connect.
        /// </summary>
        /// <returns>
        /// The System.Boolean.
        /// </returns>
        public bool CanConnect()
        {
            return true;
        }

        /// <summary>
        /// The connect.
        /// </summary>
        public void Connect()
        {
            string port = this.userSettingService.GetUserSetting<string>(UserSettingConstants.ServerPort);

            if (backgroundProcess == null)
            {
                ProcessStartInfo processStartInfo = new ProcessStartInfo(
                    "HandBrake.Server.exe", port)
                {
                    UseShellExecute = false,
                    CreateNoWindow = false,
                };

                backgroundProcess = new Process { StartInfo = processStartInfo };
                backgroundProcess.Start();
            }

            ThreadPool.QueueUserWorkItem(delegate
                {
                    try
                    {
                        pipeFactory = new DuplexChannelFactory<IServerService>(
                            new InstanceContext(this),
                            new NetTcpBinding(),
                            new EndpointAddress(string.Format("net.tcp://127.0.0.1:{0}/IHbService", port)));

                        // Connect and Subscribe to the Server
                        Service = pipeFactory.CreateChannel();
                        Service.Subscribe();
                        IsConnected = true;
                    }
                    catch (Exception exc)
                    {
                        Caliburn.Micro.Execute.OnUIThread(() => this.errorService.ShowError("Unable to connect to background worker service", "Please restart HandBrake", exc));
                    }
                });
        }

        /// <summary>
        /// The disconnect.
        /// </summary>
        public void Disconnect()
        {
            if (backgroundProcess != null && !backgroundProcess.HasExited)
            {
                try
                {
                    Service.Unsubscribe();
                }
                catch (Exception exc)
                {
                    this.errorService.ShowError("Unable to disconnect from service", "It may have already close. Check for any left over HandBrake.Server.exe processes", exc);
                }
            }
        }

        #endregion

        #region Public Service Methods

        ///// <summary>
        ///// The scan source.
        ///// </summary>
        ///// <param name="path">
        ///// The path.
        ///// </param>
        ///// <param name="title">
        ///// The title.
        ///// </param>
        ///// <param name="previewCount">
        ///// The preview count.
        ///// </param>
        //public void ScanSource(string path, int title, int previewCount)
        //{
        //    ThreadPool.QueueUserWorkItem(delegate { this.Service.ScanSource(path, title, previewCount); });
        //}

        ///// <summary>
        ///// The stop scan.
        ///// </summary>
        //public void StopScan()
        //{
        //    ThreadPool.QueueUserWorkItem(delegate { this.Service.StopScan(); });
        //}

        ///// <summary>
        ///// Start an Encode
        ///// </summary>
        ///// <param name="job">
        ///// The job.
        ///// </param>
        ///// <param name="enableLogging">
        ///// The enable logging.
        ///// </param>
        //public void StartEncode(QueueTask job, bool enableLogging)
        //{
        //    ThreadPool.QueueUserWorkItem(delegate { this.Service.StartEncode(job, enableLogging); });
        //}

        ///// <summary>
        ///// Stop an Encode
        ///// </summary>
        //public void StopEncode()
        //{
        //    ThreadPool.QueueUserWorkItem(delegate { this.Service.StopEncode(); });
        //}

        #endregion

        #region Implemented Interfaces

        #region IDisposable

        /// <summary>
        /// The dispose.
        /// </summary>
        public void Dispose()
        {
            Service.Unsubscribe();
        }

        #endregion

        #region IHbServiceCallback

        /// <summary>
        /// The scan completed.
        /// </summary>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        public virtual void ScanCompletedCallback(ScanCompletedEventArgs eventArgs)
        {
        }

        /// <summary>
        /// The scan progress.
        /// </summary>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        public virtual void ScanProgressCallback(ScanProgressEventArgs eventArgs)
        {
        }

        /// <summary>
        /// The scan started callback.
        /// </summary>
        public virtual void ScanStartedCallback()
        {
        }

        /// <summary>
        /// The encode progress callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event Args.
        /// </param>
        public virtual void EncodeProgressCallback(EncodeProgressEventArgs eventArgs)
        {
        }

        /// <summary>
        /// The encode completed callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event Args.
        /// </param>
        public virtual void EncodeCompletedCallback(EncodeCompletedEventArgs eventArgs)
        {
        }

        /// <summary>
        /// The encode started callback.
        /// </summary>
        public virtual void EncodeStartedCallback()
        {
        }

        #endregion

        #endregion
    }
}