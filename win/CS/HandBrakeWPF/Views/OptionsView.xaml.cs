// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for OptionsView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Navigation;

    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for OptionsView.xaml
    /// </summary>
    public partial class OptionsView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OptionsView"/> class.
        /// </summary>
        public OptionsView()
        {
            InitializeComponent();
        }

        private void VersionTextBox_OnMouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.versionTextBox.SelectAll();
        }

        private void ResetHandBrake_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            ((OptionsViewModel)this.DataContext).ResetHandBrake();
        }
    }
}
