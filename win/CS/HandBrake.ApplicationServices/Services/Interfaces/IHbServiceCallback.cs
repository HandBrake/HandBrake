// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IHbServiceCallback.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   HandBrake WCF Service Callbacks
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System.ServiceModel;

    using HandBrake.ApplicationServices.Services.Encode.EventArgs;

    /// <summary>
    /// HandBrake WCF Service Callbacks
    /// </summary>
    [ServiceContract]
    public interface IHbServiceCallback
    {
        /// <summary>
        /// The encode progress callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event Args.
        /// </param>
        [OperationContract(IsOneWay = true)]
        void EncodeProgressCallback(EncodeProgressEventArgs eventArgs);

        /// <summary>
        /// The encode completed callback.
        /// </summary>
        /// <param name="eventArgs">
        /// The event Args.
        /// </param>
        [OperationContract(IsOneWay = true)]
        void EncodeCompletedCallback(EncodeCompletedEventArgs eventArgs);

        /// <summary>
        /// The encode started callback.
        /// </summary>
        [OperationContract(IsOneWay = true)]
        void EncodeStartedCallback();
    }
}
