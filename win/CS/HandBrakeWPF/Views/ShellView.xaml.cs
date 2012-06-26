// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShellView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for ShellView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Interaction logic for ShellView.xaml
    /// </summary>
    public partial class ShellView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ShellView"/> class.
        /// </summary>
        public ShellView()
        {
            this.InitializeComponent();
        }

        /// <summary>
        /// Check with the user before closing.
        /// </summary>
        /// <param name="e">
        /// The CancelEventArgs.
        /// </param>
        protected override void OnClosing(System.ComponentModel.CancelEventArgs e)
        {
            bool canClose = ((IShellViewModel)this.DataContext).CanClose();
            if (!canClose)
            {
                e.Cancel = true;
            }

            base.OnClosing(e);
        }
    }
}
