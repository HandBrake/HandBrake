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

namespace HandBrake.ApplicationServices.Isolation
{
    using System;
    using System.Diagnostics;
    using System.ServiceModel;
    using System.Threading;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Services.Encode.EventArgs;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Background Service Connector.
    /// HandBrake has the ability to connect to a service app that will control HandBrakeCLI or Libhb. 
    /// This acts as process isolation.
    /// </summary>
    public class BackgroundServiceConnector : IHbServiceCallback, IDisposable
    {
        #region Constants and Fields

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
        /// <param name="port">
        /// The port.
        /// </param>
        public void Connect(string port)
        {
            if (backgroundProcess == null)
            {
                ProcessStartInfo processStartInfo = new ProcessStartInfo(
                    "HandBrake.Server.exe", port)
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    RedirectStandardOutput = true,
                };

                backgroundProcess = new Process { StartInfo = processStartInfo };
                backgroundProcess.Start();
            }

            // When the process writes out a line, it's pipe server is ready and can be contacted for
            // work. Reading line blocks until this happens.
            backgroundProcess.StandardOutput.ReadLine();

            ThreadPool.QueueUserWorkItem(delegate
                {
                    try
                    {
                        pipeFactory = new DuplexChannelFactory<IServerService>(
                            new InstanceContext(this),
                            new NetTcpBinding(),
                            new EndpointAddress(string.Format("net.tcp://127.0.0.1:{0}/IHbService", port)));

                        // Connect and Subscribe to the Server
                        this.Service = pipeFactory.CreateChannel();
                        this.Service.Subscribe();
                        this.IsConnected = true;
                    }
                    catch (Exception exc)
                    {
                        throw new GeneralApplicationException("Unable to connect to background worker process", "Please restart HandBrake", exc);
                    }
                });
        }

        /// <summary>
        /// The disconnect.
        /// </summary>
        public void Shutdown()
        {
            try
            {
                if (backgroundProcess != null && !backgroundProcess.HasExited)
                {
                    this.Service.Unsubscribe();
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException("Unable to disconnect to background worker process", 
                    "It may have already close. Check for any left over HandBrake.Server.exe processes", exc);
            }
        }

        #endregion

        #region Implemented Interfaces

        /// <summary>
        /// The dispose.
        /// </summary>
        public void Dispose()
        {
            this.Service.Unsubscribe();
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
    }
}