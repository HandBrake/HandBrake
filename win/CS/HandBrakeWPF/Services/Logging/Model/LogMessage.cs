// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogMessage.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The message.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Model
{
    /// <summary>
    /// An Immutable Log Entry.
    /// </summary>
    public class LogMessage
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LogMessage"/> class.
        /// </summary>
        /// <param name="content">
        /// The content.
        /// </param>
        /// <param name="messageIndex">
        /// The message Index.
        /// </param>
        public LogMessage(string content, long messageIndex)
        {
            this.Content = content;
            this.MessageIndex = messageIndex;
        }

        /// <summary>
        /// Gets the content.
        /// </summary>
        public string Content { get; private set; }

        /// <summary>
        /// Gets the message index.
        /// </summary>
        public long MessageIndex { get; private set; }
    }
}
