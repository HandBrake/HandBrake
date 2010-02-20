/*  UpdateCheckInformation.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Functions
{
    using System;
    using System.Threading;

    /// <summary>
    /// Provides information about an update check.
    /// </summary>
    public struct UpdateCheckInformation
    {
        /// <summary>
        /// Gets or sets a value indicating whether a New Version is Available.
        /// </summary>
        public bool NewVersionAvailable { get; set; }

        /// <summary>
        /// Gets a value indicating whether an Error Occured.
        /// </summary>
        public bool ErrorOccured
        {
            get { return this.Error != null; }
        }

        /// <summary>
        /// Gets or sets information about the new build, if any. This will be null if there is no new verison.
        /// </summary>
        public AppcastReader BuildInformation { get; set; }

        /// <summary>
        /// Gets or sets the error that occurred, if any. This will be null if no error occured.
        /// </summary>
        public Exception Error { get; set; }
    }

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
        /// </exception>
        public WaitHandle AsyncWaitHandle
        {
            get { throw new NotImplementedException(); }
        }

        /// <summary>
        /// Gets a value indicating whether CompletedSynchronously.
        /// </summary>
        /// <exception cref="NotImplementedException">
        /// </exception>
        public bool CompletedSynchronously
        {
            get { throw new NotImplementedException(); }
        }

        /// <summary>
        /// Gets a value indicating whether IsCompleted.
        /// </summary>
        /// <exception cref="NotImplementedException">
        /// </exception>
        public bool IsCompleted
        {
            get { throw new NotImplementedException(); }
        }
    }
}
