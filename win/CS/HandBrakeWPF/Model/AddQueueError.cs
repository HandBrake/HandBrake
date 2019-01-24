// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AutoNameHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model
{
    using System.Windows;

    public class AddQueueError
    {
        public string Message { get; }

        public string Header { get; }

        public MessageBoxButton Buttons { get; }

        public MessageBoxImage ErrorType { get; }

        public AddQueueError(string message, string header, MessageBoxButton buttons, MessageBoxImage errorType)
        {
            this.Message = message;
            this.Header = header;
            this.Buttons = buttons;
            this.ErrorType = errorType;
        }
    }
}