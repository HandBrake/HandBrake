// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetManagerView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for PresetManagerView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels;

    public partial class PresetManagerView : Window
    {
        public PresetManagerView()
        {
            this.InitializeComponent();
            this.Closing += this.PresetManagerView_Closing;
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }

        private void PresetManagerView_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.Closing -= this.PresetManagerView_Closing;
            ((PresetManagerViewModel)this.DataContext).Close();
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
                    ((PresetManagerViewModel)this.DataContext).SelectedPreset = (Preset)e.NewValue;
                }
                else if (e.NewValue != null && e.NewValue.GetType() == typeof(PresetDisplayCategory))
                {
                    ((PresetManagerViewModel)this.DataContext).SelectedPresetCategory = (PresetDisplayCategory)e.NewValue;
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

        private void PresetName_OnPreviewKeyDown(object sender, KeyEventArgs e)
        {
            ((PresetManagerViewModel)this.DataContext).SetPresetNameChanged();
        }

        private void Delete_Execute(object sender, ExecutedRoutedEventArgs e)
        {
            ((PresetManagerViewModel)this.DataContext).DeletePreset();
        }
    }
}
