// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PresetPaneControl.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.ViewModels;

    public partial class PresetPaneControl : UserControl
    {
        private static readonly IPresetService presetService;

        private static readonly IUserSettingService userSettingService;

        static PresetPaneControl()
        {
            presetService = IoCHelper.Get<IPresetService>();
            userSettingService = IoCHelper.Get<IUserSettingService>();
        }

        public PresetPaneControl()
        {
            InitializeComponent();
        }

        public static readonly DependencyProperty SelectedPresetProperty = DependencyProperty.Register("SelectedPreset", typeof(Preset), typeof(PresetPaneControl), new PropertyMetadata(null, OnSelectedPresetChanged));

        public Preset SelectedPreset
        {
            get
            {
                return (Preset)this.GetValue(SelectedPresetProperty);
            }

            set
            {
                this.SetValue(SelectedPresetProperty, value);
            }
        }

        private static void OnSelectedPresetChanged(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs e)
        {
            PresetPaneControl presetPane = dependencyObject as PresetPaneControl;
            Preset preset = e.NewValue as Preset;
            if (presetPane != null)
            {
                if (preset != null)
                {
                    presetService.SetSelected(preset.Name);
                }
            }
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

                PresetDisplayCategory category = treeViewItem.DataContext as PresetDisplayCategory;
                this.moveTop.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveBottom.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveUp.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveDown.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveSplitter.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        private static TreeViewItem VisualUpwardSearch(DependencyObject source)
        {
            while (source != null && !(source is TreeViewItem))
            {
                source = VisualTreeHelper.GetParent(source);
            }

            return source as TreeViewItem;
        }

        private void Delete_OnClick(object sender, RoutedEventArgs e)
        {
            Preset preset = this.presetListTree.SelectedItem as Preset;
            if (preset != null)
            {
                ((MainViewModel)this.DataContext).PresetRemove(preset);
            }
        }

        private void Delete_Execute(object sender, ExecutedRoutedEventArgs e)
        {
            Preset preset = this.presetListTree.SelectedItem as Preset;
            if (preset != null)
            {
                ((MainViewModel)this.DataContext).PresetRemove(preset);
            }
        }

        private void ContextMenu_OnOpened(object sender, RoutedEventArgs e)
        {
            Preset preset = this.presetListTree.SelectedItem as Preset;
            this.editPresetMenuItem.IsEnabled = preset == null || !preset.IsPresetDisabled;
        }

        private void PresetOptionsBtn_OnClick(object sender, RoutedEventArgs e)
        {
            var button = sender as FrameworkElement;
            if (button != null && button.ContextMenu != null)
            {
                button.ContextMenu.PlacementTarget = button;
                button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
                button.ContextMenu.IsOpen = true;
            }

            bool showPresetDesc = userSettingService.GetUserSetting<bool>(UserSettingConstants.ShowPresetDesc);
            this.presetDescMenuItem.Header = showPresetDesc ? Properties.Resources.PresetPane_HidePresetDesc : Properties.Resources.PresetPane_ShowPresetDesc;
        }
    }
}
