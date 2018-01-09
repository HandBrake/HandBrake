// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MessageDialog.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Views.Dialogs
{
    using System;
    using System.Threading.Tasks;
    using HandBrake.Model.Prompts;
    using Windows.UI.Xaml.Controls;

    public sealed partial class MessageDialog : ContentDialog
    {
        public static async Task<DialogResult> GetResult(string message, string title, DialogButtonType buttons, DialogType type)
        {
            var dlg = new MessageDialog(message, title, buttons, type);
            var result = await dlg.ShowAsync();

            switch (buttons)
            {
                case DialogButtonType.OK:
                case DialogButtonType.OKCancel:
                    return result == ContentDialogResult.Primary ? DialogResult.OK
                        : result == ContentDialogResult.Secondary ? DialogResult.Cancel
                        : DialogResult.None;

                case DialogButtonType.YesNo:
                case DialogButtonType.YesNoCancel:
                    return result == ContentDialogResult.Primary ? DialogResult.Yes
                        : result == ContentDialogResult.Secondary ? DialogResult.No
                        : DialogResult.Cancel;

                default:
                    return DialogResult.None;
            }
        }

        public static async void Show(string message, string title)
        {
            var dlg = new MessageDialog(message, title);
            await dlg.ShowAsync();
        }

        private MessageDialog(string message, string title, DialogButtonType? buttons = null, DialogType? type = null)
        {
            this.InitializeComponent();

            if (!string.IsNullOrWhiteSpace(title))
            {
                this.Title = title;
                this.Content = message;
            }
            else
            {
                this.Title = message;
            }

            var buttontype = buttons ?? DialogButtonType.OK;

            switch (buttontype)
            {
                case DialogButtonType.OK:
                case DialogButtonType.OKCancel:
                    this.PrimaryButtonText = HandBrake.Resources.Resources.OK;
                    if (buttontype == DialogButtonType.OKCancel)
                    {
                        this.SecondaryButtonText = HandBrake.Resources.Resources.Cancel;
                    }

                    break;

                case DialogButtonType.YesNo:
                case DialogButtonType.YesNoCancel:
                    this.PrimaryButtonText = HandBrake.Resources.Resources.Yes;
                    this.SecondaryButtonText = HandBrake.Resources.Resources.No;
                    break;
            }
        }
    }
}