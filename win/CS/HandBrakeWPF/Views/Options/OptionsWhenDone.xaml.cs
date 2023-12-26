// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsWhenDone.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Options
{
    using System.Windows;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model.Options;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    /// <summary>
    /// Interaction logic for OptionsWhenDone.xaml
    /// </summary>
    public partial class OptionsWhenDone : UserControl
    {
        public OptionsWhenDone()
        {
            InitializeComponent();
        }

        private void WhenDoneArguments_OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            DoDragItem(e);
        }

        private void DoDragItem(MouseButtonEventArgs e)
        {
            var mousePoint = e.GetPosition(this);
            HitTestResult result = VisualTreeHelper.HitTest(this, mousePoint);

            ListViewItem listViewItem = VisualTreeUtils.FindAncestor<ListViewItem>(result.VisualHit);
            if (listViewItem != null)
            {
                if (listViewItem.Content is PlaceHolderBucket bucket)
                {
                    DragDrop.DoDragDrop(this.SendToArguments, bucket.Name, DragDropEffects.Copy);
                }
            }
        }

        private void SendToArguments_OnDragEnter(object sender, DragEventArgs e)
        {
            e.Effects = DragDropEffects.Copy;
        }
    }
}
