/*  IErrorService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Services.Interfaces
{
    /// <summary>
    /// The Error service for showing the exception window.
    /// </summary>
    public interface IErrorService
    {
        /// <summary>
        /// Show an Error Window
        /// </summary>
        /// <param name="shortError">
        /// The short error message for the user to read
        /// </param>
        /// <param name="longError">
        /// Exception string or advanced details
        /// </param>
        void ShowError(string shortError, string longError);

        /// <summary>
        /// Show a Notice or Warning Message.
        /// </summary>
        /// <param name="notice">
        /// The text to display to the user
        /// </param>
        /// <param name="isWarning">
        /// Is a warning window, show the warning icon instead of the notice
        /// </param>
        void ShowNotice(string notice, bool isWarning);

        /// <summary>
        /// Log the error
        /// </summary>
        /// <param name="state">The error message</param>
        void LogError(object state);
    }
}