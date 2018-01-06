// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MessageLoggedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the MessageLoggedEventArgs type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.EventArgs
{
    using System;

    /// <summary>
    /// The Message Logged Event Args
    /// </summary>
    public class MessageLoggedEventArgs : EventArgs
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MessageLoggedEventArgs"/> class.
        /// </summary>
        /// <param name="message">
        /// The message.
        /// </param>
        public MessageLoggedEventArgs(string message)
        {
            this.Message = message;
        }

        /// <summary>
        /// Gets the Message.
        /// </summary>
        public string Message { get; private set; }
    }
}
