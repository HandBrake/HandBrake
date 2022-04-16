// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ListboxDeleteCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Windows.Controls;
    using System.Windows.Input;

    public class ListboxDeleteCommand : ICommand
    {
        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            ListBox listbox = parameter as ListBox;
            if (listbox != null)
            {
                IList selectedItems = new List<object>((IEnumerable<object>)listbox.SelectedItems);
                IList boundCollection = listbox.ItemsSource as IList;

                if (boundCollection != null)
                {
                    foreach (var item in selectedItems)
                    {
                        boundCollection.Remove(item);
                    }
                }
            }
        }

        public event EventHandler CanExecuteChanged;
    }
}
