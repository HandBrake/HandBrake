// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ErrorView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for ErrorView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using HandBrakeWPF.Helpers;
    using System;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;

    public partial class ErrorView : Window
    {
        public ErrorView()
        {  
            this.InitializeComponent();
            this.InputBindings.Add(new InputBinding(new CopyError(this.errorText), new KeyGesture(Key.C, ModifierKeys.Control))); // Copy Error
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }
    }

    public class CopyError : ICommand
    {
        private readonly TextBox textBlock;

        public CopyError(TextBox textBlock)
        {
            this.textBlock = textBlock;
        }

        public event EventHandler CanExecuteChanged;

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            if (this.textBlock != null)
            {
                Clipboard.SetText(this.textBlock.Text);
            }      
        }

        protected virtual void OnCanExecuteChanged()
        {
            this.CanExecuteChanged?.Invoke(this, EventArgs.Empty);
        }
    }
}
