// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceMenuCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SourceMenuCommand type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows.Input;

    /// <summary>
    /// The source menu command.
    /// </summary>
    public class SourceMenuCommand : ICommand
    {
        #region Constants and Fields

        /// <summary>
        /// The execute action.
        /// </summary>
        private readonly Action executeAction;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="SourceMenuCommand"/> class.
        /// </summary>
        /// <param name="executeAction">
        /// The execute action.
        /// </param>
        public SourceMenuCommand(Action executeAction)
        {
            this.executeAction = executeAction;
        }

        #endregion

        #region Events

        /// <summary>
        /// The can execute changed.
        /// </summary>
        public event EventHandler CanExecuteChanged;

        #endregion

        #region Implemented Interfaces

        /// <summary>
        /// The can execute.
        /// </summary>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <returns>
        /// The System.Boolean.
        /// </returns>
        public bool CanExecute(object parameter)
        {
            return true;
        }

        /// <summary>
        /// The execute.
        /// </summary>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        public void Execute(object parameter)
        {
            this.executeAction();
        }

        #endregion
    }
}