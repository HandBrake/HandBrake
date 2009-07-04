using System;

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
}
