// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetPaneControl.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels;

    public partial class PresetPaneControl : UserControl
    {
        public PresetPaneControl()
        {
            InitializeComponent();
        }

        private void PresetTreeviewItemCollapsed(object sender, RoutedEventArgs e)
        {
            if (e.Source.GetType() == typeof(TreeViewItem))
            {
                TreeViewItem item = e.Source as TreeViewItem;
                if (item != null && item.DataContext?.GetType() == typeof(PresetDisplayCategory))
                {
                    item.IsSelected = false;
                }
            }
        }

        private void PresetListTree_OnSelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            if (e.Source.GetType() == typeof(TreeView))
            {
                if (e.NewValue != null && e.NewValue.GetType() == typeof(Preset))
                {
                    ((MainViewModel)this.DataContext).SelectedPreset = (Preset)e.NewValue;
                }
            }
        }

        private void PresetListTree_OnPreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            TreeViewItem treeViewItem = VisualUpwardSearch(e.OriginalSource as DependencyObject);

            if (treeViewItem != null)
            {
                treeViewItem.Focus();
                e.Handled = true;
            }
        }

        private static TreeViewItem VisualUpwardSearch(DependencyObject source)
        {
            while (source != null && !(source is TreeViewItem))
                source = VisualTreeHelper.GetParent(source);

            return source as TreeViewItem;
        }
    }
}
