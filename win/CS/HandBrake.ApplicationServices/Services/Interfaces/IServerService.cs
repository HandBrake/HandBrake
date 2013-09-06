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
        string EncodeActivityLog { get; }

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