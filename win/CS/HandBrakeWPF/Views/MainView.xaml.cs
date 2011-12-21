// --------------------------------------------------------------------------------------------------------------------
// <copyright file="MainView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for MainView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows;

    using HandBrake.ApplicationServices.Model;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Interaction logic for MainView.xaml
    /// </summary>
    public partial class MainView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="MainView"/> class.
        /// </summary>
        public MainView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Gets ViewModel.
        /// </summary>
        private IMainViewModel ViewModel
        {
            get
            {
                return ((IMainViewModel)this.DataContext);
            }
        }

        /// <summary>
        /// Set the Selected Preset Property.
        /// The SelectedItem property of a treeview is readonly.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The RoutedPropertyChangedEventArgs.
        /// </param>
        private void TreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            this.ViewModel.SelectedPreset = e.NewValue as Preset;
        }
    }
}
