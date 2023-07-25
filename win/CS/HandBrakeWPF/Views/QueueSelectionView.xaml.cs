// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueSelectionView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for QueueSelectionView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;
    using System.Windows;
    using System.Windows.Input;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model;

    /// <summary>
    /// Interaction logic for QueueSelectionView.xaml
    /// </summary>
    public partial class QueueSelectionView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="QueueSelectionView"/> class.
        /// </summary>
        public QueueSelectionView()
        {
            InitializeComponent();
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }

        private void SelectionGrid_OnKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Space && this.SelectionGrid.SelectedItems.Count == 1)
            {
                SelectionTitle title = this.SelectionGrid.SelectedItems[0] as SelectionTitle;
                if (title != null)
                {
                    title.IsSelected = !title.IsSelected;
                }
            }
        }
    }
}
