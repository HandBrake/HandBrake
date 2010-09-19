/*  UpdateCheckResult.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Model
{
    using System;
    using System.Threading;

    /// <summary>
    /// Used in EndUpdateCheck() for update checking and the IAsyncResult design pattern.
    /// </summary>
    public class UpdateCheckResult : IAsyncResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HandBrake.Framework.Model.UpdateCheckResult"/> class.
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
