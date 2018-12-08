// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueCommands.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The queue commands.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.Menu
{
    using System;
    using System.Windows.Input;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The queue commands.
    /// </summary>
    public class QueueCommands : ICommand
    {
        /// <summary>
        /// Gets or sets the queue view model.
        /// </summary>
        public IQueueViewModel QueueViewModel { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueCommands"/> class. 
        /// </summary>
        /// <param name="queueViewModel">
        /// The queue View Model.
        /// </param>
        public QueueCommands(IQueueViewModel queueViewModel)
        {
            this.QueueViewModel = queueViewModel;
        }

        /// <summary>
        /// Defines the method that determines whether the command can execute in its current state.
        /// </summary>
        /// <returns>
        /// true if this command can be executed; otherwise, false.
        /// </returns>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public bool CanExecute(object parameter)
        {
            return true;
        }

        /// <summary>
        /// Defines the method to be called when the command is invoked.
        /// </summary>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public void Execute(object parameter)
        {
            switch ((QueueCommandParams)parameter)
            {
                case QueueCommandParams.ClearAll:
                    this.QueueViewModel.Clear();
                    break;
                case QueueCommandParams.ClearCompleted:
                    this.QueueViewModel.ClearCompleted();
                    break;
                case QueueCommandParams.ClearSelected:
                    this.QueueViewModel.RemoveSelectedJobs();
                    break;
                case QueueCommandParams.Import:
                    this.QueueViewModel.Import();
                    break;
                case QueueCommandParams.Export:
                    this.QueueViewModel.Export();
                    break;
            }
        }

        /// <summary>
        /// The can execute changed.
        /// </summary>
        public event EventHandler CanExecuteChanged { add { } remove { } }
    }
}
