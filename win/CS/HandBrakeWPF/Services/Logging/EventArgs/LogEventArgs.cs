// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the LogEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.EventArgs
{
    using System;

    using HandBrakeWPF.Model.Logging;

    /// <summary>
    /// The Message Logged Event Args
    /// </summary>
    public class LogEventArgs : EventArgs
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LogEventArgs"/> class.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        public LogEventArgs(LogMessage message)
        {
            this.Log = message;
        }

        /// <summary>
        /// Gets the Message.
        /// </summary>
        public LogMessage Log { get; private set; }
    }
}
