// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceSelection.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for SourceSelection.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System.Windows.Controls;
    using System.Windows.Navigation;

    using HandBrakeWPF.ViewModels;

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

        private void AboutHandBrake_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            MainViewModel vm = this.DataContext as MainViewModel;
            vm.OpenAboutApplication();
        }

        private void OpenOptions_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            MainViewModel vm = this.DataContext as MainViewModel;
            vm.OpenOptionsWindow();
        }
    }
}
