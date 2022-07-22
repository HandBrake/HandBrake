// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SimpleRelayCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows.Input;

    public class SimpleRelayCommand<T> : ICommand
    {
        private Action<T> command;
        private Func<bool> canExecute;

        public SimpleRelayCommand(Action<T> commandAction, Func<bool> canExecute = null)
        {
            this.command = commandAction;
            this.canExecute = canExecute;
        }

        public bool CanExecute(object parameter)
        {
            return this.canExecute == null ? true : this.canExecute();
        }

        public void Execute(object parameter)
        {
            if (this.command != null)
            {
                if (typeof(T) == typeof(int))
                {
                    parameter = int.Parse(parameter.ToString() ?? string.Empty);
                }

                this.command((T)parameter);
            }
        }

        public event EventHandler CanExecuteChanged;
    }
}
