// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OptionsOutput.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Options
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Model.Options;

    public partial class OptionsOutput : UserControl
    {
        public OptionsOutput()
        {
            InitializeComponent();
        }

        private void FileFormatChoices_OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            DoDragItem(e);
        }

        private void PathFormatChoices_OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            DoDragItem(e);
        }

        private void AutoNameFormat_OnDragEnter(object sender, DragEventArgs e)
        {
            e.Effects = DragDropEffects.Copy;
        }

        private void AutoNameOutputPath_OnDragEnter(object sender, DragEventArgs e)
        {
            e.Effects = DragDropEffects.Copy;
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
                    DragDrop.DoDragDrop(this.fileFormatChoices, bucket.Name, DragDropEffects.Copy);
                }
            }
        }
    }
}
