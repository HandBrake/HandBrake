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
    using System.Runtime.Serialization;
    using System.ServiceModel;
    using System.Windows;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Parsing;
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
        /// The scan service.
        /// </summary>
        private static IScan scanService;

        /// <summary>
        /// The host.
        /// </summary>
        private ServiceHost host;

        #endregion

        #region Implemented Interfaces

        #region IServerService

        /// <summary>
        /// The scan source.
        /// </summary>
        /// <param name="path">
        /// The path.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        public void ScanSource(string path, int title, int previewCount)
        {
            Console.WriteLine("Starting Source Scan for: " + path);
            scanService.ScanStared += this.ScanStaredHandler;
            scanService.ScanStatusChanged += this.ScanStatusChangedHandler;
            scanService.ScanCompleted += this.ScanCompletedHandler;

            scanService.Scan(path, title, previewCount, null);
        }

        /// <summary>
        /// Gets the activity log.
        /// </summary>
        [DataMember]
        public string ActivityLog
        {
            get
            {
                return scanService.ActivityLog;

            }
        }

        /// <summary>
        /// Gets the souce data.
        /// </summary>
        [DataMember]
        public Source SouceData
        {
            get
            {
                return scanService.SouceData;
            }
        }

        /// <summary>
        /// Gets a value indicating whether is scanning.
        /// </summary>
        [DataMember]
        public bool IsScanning
        {
            get
            {
                return scanService.IsScanning;
            }
        }

        /// <summary>
        /// Start the service
        /// </summary>
        public void Start()
        {
            using (this.host = new ServiceHost(typeof(ServerService), new Uri("net.tcp://localhost:8000")))
            {
                // Setup a listener
                this.host.AddServiceEndpoint(typeof(IServerService), new NetTcpBinding(), "IHbService");
                this.host.Open();
                Console.WriteLine("::: HandBrake Server :::");
                Console.WriteLine("Service Started");

                // Setup the services we are going to use.
                scanService = new ScanService(new UserSettingService());
                Console.ReadLine();
            }
        }

        /// <summary>
        /// Stop this service
        /// </summary>
        public void Stop()
        {
            if (this.host != null)
            {
                this.host.Close();
                Application.Current.Shutdown();
            }
        }

        /// <summary>
        /// Stop the scan.
        /// </summary>
        public void StopScan()
        {
            scanService.Stop();
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
        /// The scan service scan completed event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ScanCompletedHandler(object sender, ScanCompletedEventArgs e)
        {
            Subscribers.ForEach(
                delegate(IHbServiceCallback callback)
                {
                    if (((ICommunicationObject)callback).State == CommunicationState.Opened)
                    {
                        Console.WriteLine("Scan Completed Callback");
                        callback.ScanCompletedCallback(e);
                    }
                    else
                    {
                        Subscribers.Remove(callback);
                    }
                });

            scanService.ScanStared -= this.ScanStaredHandler;
            scanService.ScanStatusChanged -= this.ScanStatusChangedHandler;
            scanService.ScanCompleted -= this.ScanCompletedHandler;
        }

        /// <summary>
        /// The scan service scan stared.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ScanStaredHandler(object sender, EventArgs e)
        {
            Subscribers.ForEach(
                delegate(IHbServiceCallback callback)
                {
                    if (((ICommunicationObject)callback).State == CommunicationState.Opened)
                    {
                        Console.WriteLine("Scan Started Callback");
                        callback.ScanStartedCallback();
                    }
                    else
                    {
                        Subscribers.Remove(callback);
                    }
                });
        }

        /// <summary>
        /// The scan service scan status changed event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ScanStatusChangedHandler(object sender, ScanProgressEventArgs e)
        {
            Subscribers.ForEach(
                delegate(IHbServiceCallback callback)
                {
                    if (((ICommunicationObject)callback).State == CommunicationState.Opened)
                    {
                        Console.WriteLine("Scan Changed Callback");
                        callback.ScanProgressCallback(e);
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