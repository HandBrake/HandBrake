// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The log service.
//   For now, this is just a simple logging service but we could provide support for a formal logging library later.
//   Also, we can consider providing the UI layer with more functional logging. (i.e levels, time/date, highlighting etc)
//   The Interop Classes are not very OO friendly, so this is going to be a static class.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Logging
{
    using System.Diagnostics;

    using HandBrake.ApplicationServices.Services.Logging.Model;

    /// <summary>
    /// The log helper.
    /// </summary>
    public static class LogHelper
    {
        private static LogLevel currentLogLevel = LogLevel.debug; // TODO default to Info when this class is implimented. 

        /// <summary>
        /// Log message.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        public static void LogMessage(LogMessage message)
        {
            if (message.LogLevel <= currentLogLevel)
            {
                Debug.WriteLine(message.Content);
            }

            // TODO cache logging.         
        }

        /// <summary>
        /// The set log level. Default: Info.
        /// </summary>
        /// <param name="level">
        /// The level.
        /// </param>
        public static void SetLogLevel(LogLevel level)
        {
            currentLogLevel = level;
        }
    }
}
