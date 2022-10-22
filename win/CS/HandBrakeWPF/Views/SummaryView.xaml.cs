// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SummaryView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for SummaryView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Windows.Controls;
    using System.Windows.Input;

    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Interaction logic for SummaryView.xaml
    /// </summary>
    public partial class SummaryView : UserControl
    {
        public SummaryView()
        {
            this.InitializeComponent();
        }

        private void PreviewImage_OnMouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (e.Delta > 1)
            {
                ((ISummaryViewModel)this.DataContext).NextPreview();
            }
            else
            {
                ((ISummaryViewModel)this.DataContext).PreviousPreview();
            }
        }

        private void Previous_OnMouseDown(object sender, MouseButtonEventArgs e)
        {
            ((SummaryViewModel)this.DataContext).PreviousPreview();
        }

        private void Next_OnMouseDown(object sender, MouseButtonEventArgs e)
        {
            ((SummaryViewModel)this.DataContext).NextPreview();
        }
    }
}
