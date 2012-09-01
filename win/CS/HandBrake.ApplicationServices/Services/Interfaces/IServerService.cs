// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IServerService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IServerService type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System.Runtime.Serialization;
    using System.ServiceModel;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// The HandBrakeService interface.
    /// </summary>
    [ServiceContract(CallbackContract = typeof(IHbServiceCallback), SessionMode = SessionMode.Required)]
    public interface IServerService
    {
        /// <summary>
        /// Gets the activity log.
        /// </summary>
        [DataMember]
        string ScanActivityLog { get; }

        /// <summary>
        /// Gets the activity log.
        /// </summary>
        [DataMember]
        string EncodeActivityLog { get; }

        /// <summary>
        /// Gets the souce data.
        /// </summary>
        Source SouceData
        {
            [OperationContract]
            get;
        }

        /// <summary>
        /// Gets a value indicating whether is scanning.
        /// </summary>
        bool IsScanning
        {
            [OperationContract]
            get;
        }

        /// <summary>
        /// Gets a value indicating whether is encoding.
        /// </summary>
        bool IsEncoding
        {
            [OperationContract]
            get;
        }

        /// <summary>
        /// Start the WCF Service
        /// </summary>
        /// <param name="port">
        /// The port.
        /// </param>
        void Start(string port);

        /// <summary>
        /// Stop the WCF Service
        /// </summary>
        void Stop();

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
        [OperationContract]
        void ScanSource(string path, int title, int previewCount);

        /// <summary>
        /// Start and Encode
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="enableLogging">
        /// The enable logging.
        /// </param>
        [OperationContract]
        void StartEncode(QueueTask job, bool enableLogging);

        /// <summary>
        /// The process encode logs.
        /// </summary>
        /// <param name="destination">
        /// The destination.
        /// </param>
        [OperationContract]
        void ProcessEncodeLogs(string destination);

        /// <summary>
        /// Stop and Encode
        /// </summary>
        [OperationContract]
        void StopEncode();

        /// <summary>
        /// Stop the scan.
        /// </summary>
        [OperationContract]
        void StopScan();

        /// <summary>
        /// Subscribe for callbacks from the called functions
        /// </summary>
        /// <returns>
        /// The System.Boolean.
        /// </returns>
        [OperationContract]
        bool Subscribe();

        /// <summary>
        /// Unsubscribe from callbacks.
        /// </summary>
        /// <returns>
        /// The System.Boolean.
        /// </returns>
        [OperationContract]
        bool Unsubscribe();
    }
}