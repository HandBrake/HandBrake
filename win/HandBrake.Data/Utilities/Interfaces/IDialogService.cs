// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IDialogService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An interface for Creating Dialog Prompts.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities.Interfaces
{
    using HandBrake.Model.Prompts;

    /// <summary>
    /// An interface for Creating Dialog Prompts.
    /// </summary>
    public interface IDialogService
    {
        /// <summary>
        /// Shows a Dialog and gets the result.
        /// </summary>
        /// <param name="message">Message for the Dialog.</param>
        /// <param name="header">Header for the Dialog.</param>
        /// <param name="buttons">Buttons for the Dialog.</param>
        /// <param name="type">Type of Dialog</param>
        /// <returns>Result of Dialog.</returns>
        DialogResult Show(string message, string header, DialogButtonType buttons, DialogType type);

        /// <summary>
        /// Shows a Dialog.
        /// </summary>
        /// <param name="message">Message for the Dialog.</param>
        /// <param name="title">Title of Dialog.</param>
        void Show(string message, string title);

        /// <summary>
        /// Shows a Dialog.
        /// </summary>
        /// <param name="message">Message for the Dialog.</param>
        void Show(string message);
    }
}