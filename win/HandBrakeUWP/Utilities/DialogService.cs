// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DialogService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using System;
    using System.Threading.Tasks;
    using HandBrake.Model.Prompts;
    using HandBrake.Utilities.Interfaces;
    using HandBrake.Views.Dialogs;
    using Execute = Caliburn.Micro.Execute;

    public class DialogService : IDialogService
    {
        public DialogResult Show(string message, string title, DialogButtonType buttons, DialogType type)
        {
            TaskCompletionSource<DialogResult> waiter = new TaskCompletionSource<DialogResult>();
            Execute.OnUIThread(async () =>
            {
                var result = await MessageDialog.GetResult(message, title, buttons, type);
                waiter.TrySetResult(result);
            });
            return waiter.Task.Result;
        }

        public void Show(string message)
        {
            this.Show(message, null);
        }

        public void Show(string message, string title)
        {
            Execute.OnUIThread(() =>
            {
                MessageDialog.Show(message, title);
            });
        }
    }
}