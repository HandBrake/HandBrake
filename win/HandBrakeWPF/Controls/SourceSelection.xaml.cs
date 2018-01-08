// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceSelection.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for SourceSelection.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Controls
{
    using System.Diagnostics;
    using System.Windows.Controls;
    using System.Windows.Navigation;

    using HandBrake.ViewModels;

    /// <summary>
    /// Interaction logic for SourceSelection.xaml
    /// </summary>
    public partial class SourceSelection : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SourceSelection"/> class. 
        /// </summary>
        public SourceSelection()
        {
            this.InitializeComponent();
        }

        /// <summary>
        /// The about request navigate.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void AboutHandBrake_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            MainViewModelBase vm = this.DataContext as MainViewModelBase;
            vm.OpenAboutApplication();
        }

        /// <summary>
        /// The open options request navigate.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void OpenOptions_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            MainViewModelBase vm = this.DataContext as MainViewModelBase;
            vm.OpenOptionsWindow();
        }

        /// <summary>
        /// The help_ on request navigate.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void Help_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            Process.Start("https://handbrake.fr/docs");
        }
    }
}
