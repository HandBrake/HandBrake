using System;
using System.Threading;

namespace Handbrake.Functions
{
    /// <summary>
    /// Provides information about an update check.
    /// </summary>
    public struct UpdateCheckInformation
    {
        public bool NewVersionAvailable { get; set; }
        public bool ErrorOccured { get { return Error != null; } }

        /// <summary>
        /// Gets information about the new build, if any. This will be null if there is no new verison.
        /// </summary>
        public AppcastReader BuildInformation { get; set; }

        /// <summary>
        /// Gets the error that occurred, if any. This will be null if no error occured.
        /// </summary>
        public Exception Error { get; set; }
    }

    /// <summary>
    /// Used in EndUpdateCheck() for update checking and the IAsyncResult design pattern.
    /// </summary>
    public class UpdateCheckResult : IAsyncResult
    {
        public UpdateCheckResult(object asyncState, UpdateCheckInformation info)
        {
            AsyncState = asyncState;
            Result = info;
        }

        /// <summary>
        /// Gets whether the check was executed in debug mode.
        /// </summary>
        public object AsyncState { get; private set; }

        /// <summary>
        /// Gets the result of the update check.
        /// </summary>
        public UpdateCheckInformation Result { get; private set; }

        public WaitHandle AsyncWaitHandle { get { throw new NotImplementedException(); } }
        public bool CompletedSynchronously { get { throw new NotImplementedException(); } }
        public bool IsCompleted { get { throw new NotImplementedException(); } }
    }
}
