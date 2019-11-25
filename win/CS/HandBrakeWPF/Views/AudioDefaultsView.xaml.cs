// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioDefaultsView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AudioDefaultsView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;
    using System.Windows.Controls;

    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for AudioDefaultsView.xaml
    /// </summary>
    public partial class AudioDefaultsView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AudioDefaultsView"/> class.
        /// </summary>
        public AudioDefaultsView()
        {
            this.InitializeComponent();
        }

        private void Apply_OnClick(object sender, RoutedEventArgs e)
        {
            ((AudioDefaultsViewModel)DataContext).IsApplied = true;
            this.Close();
        }
    }
}
