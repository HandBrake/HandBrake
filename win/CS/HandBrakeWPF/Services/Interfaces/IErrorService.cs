// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IErrorService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Error Service Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

using System.Windows;

namespace HandBrakeWPF.Services.Interfaces
{
    public interface IErrorService
    {
        /// <summary>
        /// Show an Error Window with debug output.
        /// </summary>
        /// <param name="message"></param>
        /// <param name="solution"></param>
        /// <param name="details"></param>
        void ShowError(string message, string solution, string details);

        /// <summary>
        /// Show a Message Box
        /// </summary>
        /// <param name="message"></param>
        /// <param name="header"></param>
        /// <param name="image"></param>
        /// <param name="buttons"></param>
        /// <returns></returns>
        MessageBoxResult ShowMessageBox(string message, string header, MessageBoxButton buttons, MessageBoxImage image);
    }
}