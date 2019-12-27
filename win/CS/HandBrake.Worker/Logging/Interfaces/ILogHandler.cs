// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ILogHandler.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ILogHandler type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Logging.Interfaces
{
    using HandBrake.Worker.Logging.Models;

    public interface ILogHandler
    {
        /// <summary>
        /// Enable logging for this worker process.
        /// </summary>
        /// <param name="config">
        /// Configuration for the logger.
        /// </param>
        /// <remarks>
        /// If this is not called, all log messages from libhb will be ignored.
        /// </remarks>
        void ConfigureLogging(LogHandlerConfig config);

        string GetFullLog();

        long GetLatestLogIndex();

        /// <summary>
        /// Get the log data from a given index
        /// </summary>
        /// <param name="index">index is zero based</param>
        /// <returns>Full log as a string</returns>
        string GetLogFromIndex(int index);

        /// <summary>
        /// Empty the log cache and reset the log handler to defaults.
        /// </summary>
        void Reset();
    }
}
