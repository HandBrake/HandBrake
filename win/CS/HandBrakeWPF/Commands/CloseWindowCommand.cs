// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CloseWindowCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows;
    using System.Windows.Input;

    public class CloseWindowCommand : ICommand
    {
        private Window w;

        public CloseWindowCommand(Window w)
        {
            this.w = w;
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            if (w != null)
            {
                w.Close();
            }
        }

        public event EventHandler CanExecuteChanged;
    }
}
