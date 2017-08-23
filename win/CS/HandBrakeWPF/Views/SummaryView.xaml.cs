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

    using HandBrakeWPF.ViewModels.Interfaces;

    using Image = System.Windows.Controls.Image;
    using Point = System.Windows.Point;

    /// <summary>
    /// Interaction logic for SummaryView.xaml
    /// </summary>
    public partial class SummaryView : UserControl
    {
        public SummaryView()
        {
            this.InitializeComponent();
        }

        private void PreviewImage_OnMouseMove(object sender, MouseEventArgs e)
        {
            Image image = sender as Image;

            if (image != null && image.ActualWidth > 0)
            {
                Point p = Mouse.GetPosition(image);
                double width = image.ActualWidth / 2;

                bool leftHalf = p.X <= width;
                bool rightHalf = p.X > width;

                ((ISummaryViewModel)this.DataContext).SetPreviewControlVisibility(leftHalf, rightHalf);
            }
        }
    }
}
