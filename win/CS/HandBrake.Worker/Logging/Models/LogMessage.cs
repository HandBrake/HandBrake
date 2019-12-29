// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogMessage.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Immutable log message
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Logging.Models
{
    public class LogMessage
    {
        public LogMessage(string content, int messageIndex)
        {
            this.Content = content;
            this.MessageIndex = messageIndex;
        }

        public string Content { get; private set; }

        public int MessageIndex { get; private set; }
    }
}
