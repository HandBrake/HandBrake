// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ServerService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrake WCF Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.ServiceModel;
    using System.Threading;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode;
    using HandBrake.ApplicationServices.Services.Encode.EventArgs;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// HandBrake WCF Service
    /// </summary>
    [ServiceBehavior(InstanceContextMode = InstanceContextMode.PerCall, IncludeExceptionDetailInFaults = true, ConcurrencyMode = ConcurrencyMode.Single)]
    public class ServerService : IServerService
    {
        #region Constants and Fields

        /// <summary>
        /// List of connected Clients. For now, this should only be one.
        /// </summary>
        private static readonly List<IHbServiceCallback> Subscribers = new List<IHbServiceCallback>();

        /// <summary>
        /// The encode service.
        /// </summary>
        private static IEncode encodeService;

        /// <summary>
        /// The host.
        /// </summary>
        private static ServiceHost host;

        /// <summary>
        /// The shutdown flag.
        /// </summary>
        private static ManualResetEvent shutdownFlag;

        #endregion

        #region Properties

        /// <summary>
        /// Gets the activity log.
        /// </summary>
        public string EncodeActivityLog { get; private set; }

        /// <summary>
        /// Gets a value indicating whether is encoding.
        /// </summary>
        public bool IsEncoding { get; private set; }

        /// <summary>
        /// Gets the activity log.
        /// </summary>
        public string ScanActivityLog { get; private set; }

        #endregion

        #region Implemented Interfaces

        #region IServerService

        /// <summary>
        /// The process encode logs.
        /// </summary>
        /// <param name="destination">
        /// The destination.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        public void ProcessEncodeLogs(string destination, HBConfiguration configuration)
        {
            encodeService.ProcessLogs(destination, configuration);
        }

        /// <summary>
        /// Start the service
        /// </summary>
        /// <param name="port">
        /// The port.
        /// </param>
        public void Start(string port)
        {
            using (host = new ServiceHost(typeof(ServerService), new Uri(string.Format("net.tcp://127.0.0.1:{0}", port))))
            {
                // Setup a listener
                host.AddServiceEndpoint(typeof(IServerService), new NetTcpBinding(), "IHbService");
                host.Open();
                Console.WriteLine("::: HandBrake Isolation Server - Debug Console:::");
                Console.WriteLine("Service Started. Waiting for Clients...");

                // Setup the services we are going to use.
                encodeService = new LibEncode(); 
                shutdownFlag = new ManualResetEvent(false);
                shutdownFlag.WaitOne();
            }
        }

        /// <summary>
        /// Start and Encode
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        public void StartEncode(QueueTask job)
        {
            Console.WriteLine("Starting Source Encode for: " + job.Task.Source);
            encodeService.EncodeCompleted += this.EncodeServiceEncodeCompleted;
            encodeService.EncodeStarted += this.encodeService_EncodeStarted;
            encodeService.EncodeStatusChanged += this.encodeService_EncodeStatusChanged;
            encodeService.Start(job);
        }

        /// <summary>
        /// Stop this service
        /// </summary>
        public void Stop()
        {
            if (host != null)
            {
                host.BeginClose(null, null);
                // host.Abort();            
                shutdownFlag.Set();
            }
        }

        /// <summary>
        /// Stop and Encode
        /// </summary>
        public void StopEncode()
        {
            encodeService.Stop();
        }

        /// <summary>
        /// The subscribe.
        /// </summary>
        /// <returns>
        /// The System.Boolean.
        /// </returns>
        public bool Subscribe()
        {
            try
            {
                // Get the hashCode of the connecting app and store it as a connection
                var callback = OperationContext.Current.GetCallbackChannel<IHbServiceCallback>();
                if (!Subscribers.Contains(callback))
                {
                    Console.WriteLine("Client Connected");
                    Subscribers.Add(callback);
                }
                return true;
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return false;
            }
        }

        /// <summary>
        /// The unsubscribe.
        /// </summary>
        /// <returns>
        /// The System.Boolean.
        /// </returns>
        public bool Unsubscribe()
        {
            try
            {
                var callback = OperationContext.Current.GetCallbackChannel<IHbServiceCallback>();
                if (Subscribers.Contains(callback))
                {
                    Subscribers.Remove(callback);
                    if (Subscribers.Count == 0)
                    {
                        Console.WriteLine("Client Disconnected, Shutting down...");

                        // Shutdown the service. We no longer have any clients to serve.
                        // It is the responsibility of the UI to maintain a subscription while this service is in use.
                        this.Stop();
                    }
                }
                return true;
            }
            catch
            {
                return false;
            }
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
        /// The e.
        /// </param>
        private void EncodeServiceEncodeCompleted(object sender, EncodeCompletedEventArgs e)
        {
            encodeService.EncodeCompleted -= this.EncodeServiceEncodeCompleted;
            encodeService.EncodeStarted -= this.encodeService_EncodeStarted;
            encodeService.EncodeStatusChanged -= this.encodeService_EncodeStatusChanged;

            Subscribers.ForEach(
                delegate(IHbServiceCallback callback)
                    {
                        if (((ICommunicationObject)callback).State == CommunicationState.Opened)
                        {
                            Console.WriteLine("Encode Completed Callback");
                            callback.EncodeCompletedCallback(e);
                        }
                        else
                        {
                            Subscribers.Remove(callback);
                        }
                    });
        }

        /// <summary>
        /// The encode service_ encode started.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void encodeService_EncodeStarted(object sender, EventArgs e)
        {
            Subscribers.ForEach(
                delegate(IHbServiceCallback callback)
                    {
                        if (((ICommunicationObject)callback).State == CommunicationState.Opened)
                        {
                            Console.WriteLine("Encode Started Callback");
                            callback.EncodeStartedCallback();
                        }
                        else
                        {
                            Subscribers.Remove(callback);
                        }
                    });
        }

        /// <summary>
        /// The encode service_ encode status changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void encodeService_EncodeStatusChanged(object sender, EncodeProgressEventArgs e)
        {
            Subscribers.ForEach(
                delegate(IHbServiceCallback callback)
                    {
                        if (((ICommunicationObject)callback).State == CommunicationState.Opened)
                        {
                            Console.WriteLine("Encode Status Callback");
                            callback.EncodeProgressCallback(e);
                        }
                        else
                        {
                            Subscribers.Remove(callback);
                        }
                    });
        }

        #endregion
    }
}