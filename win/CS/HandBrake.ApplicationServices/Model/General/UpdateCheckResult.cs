// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UpdateCheckResult.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Used in EndUpdateCheck() for update checking and the IAsyncResult design pattern.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model.General
{
    using System;
    using System.Threading;

    /// <summary>
    /// Used in EndUpdateCheck() for update checking and the IAsyncResult design pattern.
    /// </summary>
    public class UpdateCheckResult : IAsyncResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UpdateCheckResult"/> class.
        /// </summary>
        /// <param name="asyncState">
        /// The async state.
        /// </param>
        /// <param name="info">
        /// The info.
        /// </param>
        public UpdateCheckResult(object asyncState, UpdateCheckInformation info)
        {
            this.AsyncState = asyncState;
            this.Result = info;
        }

        /// <summary>
        /// Gets whether the check was executed in debug mode.
        /// </summary>
        public object AsyncState { get; private set; }

        /// <summary>
        /// Gets the result of the update check.
        /// </summary>
        public UpdateCheckInformation Result { get; private set; }

        /// <summary>
        /// Gets AsyncWaitHandle.
        /// </summary>
        /// <exception cref="NotImplementedException">
        /// This is not implemented as it is not used.
        /// </exception>
        public WaitHandle AsyncWaitHandle
        {
            get { throw new NotImplementedException(); }
        }

        /// <summary>
        /// Gets a value indicating whether CompletedSynchronously.
        /// </summary>
        /// <exception cref="NotImplementedException">
        /// This is not implemented as it is not used.
        /// </exception>
        public bool CompletedSynchronously
        {
            get { throw new NotImplementedException(); }
        }

        /// <summary>
        /// Gets a value indicating whether IsCompleted.
        /// </summary>
        /// <exception cref="NotImplementedException">
        /// This is not implemented as it is not used.
        /// </exception>
        public bool IsCompleted
        {
            get { throw new NotImplementedException(); }
        }
    }
}
