// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ProcessShortcutCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Keyboard Shortcut Processor
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows.Input;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Keyboard Shortcut Processor
    /// </summary>
    public class ProcessQueueShortcutCommand : ICommand
    {
        /// <summary>
        /// The Gesture
        /// </summary>
        private readonly KeyGesture gesture;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessShortcutCommand"/> class.
        /// </summary>
        /// <param name="gesture">
        /// The gesture.
        /// </param>
        public ProcessQueueShortcutCommand(KeyGesture gesture)
        {
            this.gesture = gesture;
        }

        /// <summary>
        /// Defines the method to be called when the command is invoked.
        /// </summary>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public void Execute(object parameter)
        {
            if (gesture != null)
            {
                QueueViewModel vm = (QueueViewModel)IoCHelper.Get<IQueueViewModel>();
                
                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.P)
                {
                    vm.PlayFile();
                }

                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.I)
                {
                    vm.OpenSourceDir();
                }

                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.D)
                {
                    vm.OpenDestDir();
                }

                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.E)
                {
                    vm.EditSelectedJob();
                }

                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.R)
                {
                    vm.ResetSelectedJobs();
                }

                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.T)
                {
                    vm.MoveToTop();
                }

                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.B)
                {
                    vm.MoveToBottom();
                }
            }
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
        /// Can Execute Changed
        /// </summary>
        public event EventHandler CanExecuteChanged { add { } remove { } }
    }
}
