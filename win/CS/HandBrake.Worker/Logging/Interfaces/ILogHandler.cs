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
    using System.Collections.Generic;

    using HandBrake.Worker.Logging.Models;

    public interface ILogHandler
    {
        string GetFullLog();

        List<LogMessage> GetLogMessages();

        /// <summary>
        /// Get the log data from a given index
        /// </summary>
        /// <param name="index">index is zero based</param>
        /// <returns>Full log as a string</returns>
        List<LogMessage> GetLogMessagesFromIndex(int index);

        /// <summary>
        /// Empty the log cache and reset the log handler to defaults.
        /// </summary>
        void Reset();

        void ShutdownFileWriter();
    }
}
