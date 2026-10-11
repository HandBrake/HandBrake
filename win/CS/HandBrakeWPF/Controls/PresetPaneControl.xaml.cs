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
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Linq;
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

        private readonly ObservableCollection<IPresetObject> rows = new ObservableCollection<IPresetObject>();

        private readonly List<INotifyPropertyChanged> trackedItems = new List<INotifyPropertyChanged>();

        private readonly List<IBindingList> trackedLists = new List<IBindingList>();

        private INotifyPropertyChanged trackedViewModel;

        private bool rebuildPending;

        private bool isSyncing;

        private readonly System.Windows.Threading.DispatcherTimer searchTimer;

        private string searchText = string.Empty;

        public PresetPaneControl()
        {
            InitializeComponent();

            this.searchTimer = new System.Windows.Threading.DispatcherTimer { Interval = TimeSpan.FromMilliseconds(250) };
            this.searchTimer.Tick += this.SearchTimer_OnTick;
            this.Unloaded += (s, e) => this.searchTimer.Stop();

            this.presetListView.ItemsSource = this.rows;
            this.DataContextChanged += this.OnDataContextChanged;
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

        private void OnDataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.trackedViewModel != null)
            {
                this.trackedViewModel.PropertyChanged -= this.ViewModel_PropertyChanged;
            }

            this.trackedViewModel = e.NewValue as INotifyPropertyChanged;
            if (this.trackedViewModel != null)
            {
                this.trackedViewModel.PropertyChanged += this.ViewModel_PropertyChanged;
            }

            this.ScheduleRebuild();
        }

        private bool IsSearchActive => !string.IsNullOrEmpty(this.searchText);

        private void PresetSearchBox_OnTextChanged(object sender, TextChangedEventArgs e)
        {
            this.searchTimer.Stop();
            if (string.IsNullOrWhiteSpace(this.presetSearchBox.Text))
            {
                this.ApplySearch();
                return;
            }

            this.searchTimer.Start();
        }

        private void SearchTimer_OnTick(object sender, EventArgs e)
        {
            this.searchTimer.Stop();
            this.ApplySearch();
        }

        private void ApplySearch()
        {
            string text = (this.presetSearchBox.Text ?? string.Empty).Trim();
            if (text == this.searchText)
            {
                return;
            }

            this.searchText = text;
            this.ScheduleRebuild();
        }

        private void PresetSearchClear_OnClick(object sender, RoutedEventArgs e)
        {
            this.presetSearchBox.Clear();
            this.presetSearchBox.Focus();
        }

        private bool MatchesSearch(Preset preset)
        {
            return (preset.Name != null && preset.Name.Contains(this.searchText, StringComparison.CurrentCultureIgnoreCase))
                   || (preset.Description != null && preset.Description.Contains(this.searchText, StringComparison.CurrentCultureIgnoreCase));
        }

        private void ViewModel_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(MainViewModel.PresetsCategories) || e.PropertyName == nameof(MainViewModel.SelectedPreset))
            {
                this.ScheduleRebuild();
            }
        }

        private void Tracked_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(IPresetObject.IsExpanded))
            {
                this.ScheduleRebuild();
            }
            else if (e.PropertyName == nameof(IPresetObject.IsSelected))
            {
                this.SyncSelection();
            }
        }

        private void SyncSelection()
        {
            bool previous = this.isSyncing;
            this.isSyncing = true;
            try
            {
                Preset selected = this.rows.OfType<Preset>().FirstOrDefault(p => p.IsSelected);
                if (!ReferenceEquals(this.presetListView.SelectedItem, selected))
                {
                    this.presetListView.SelectedItem = selected;
                }

                if (selected != null)
                {
                    this.presetListView.ScrollIntoView(selected);
                }
            }
            finally
            {
                this.isSyncing = previous;
            }
        }

        private void TrackedList_ListChanged(object sender, ListChangedEventArgs e)
        {
            this.ScheduleRebuild();
        }

        private void ScheduleRebuild()
        {
            if (this.rebuildPending)
            {
                return;
            }

            this.rebuildPending = true;
            this.Dispatcher.BeginInvoke(new Action(this.Rebuild), System.Windows.Threading.DispatcherPriority.Background);
        }

        private void Untrack()
        {
            foreach (INotifyPropertyChanged item in this.trackedItems)
            {
                item.PropertyChanged -= this.Tracked_PropertyChanged;
            }

            foreach (IBindingList list in this.trackedLists)
            {
                list.ListChanged -= this.TrackedList_ListChanged;
            }

            this.trackedItems.Clear();
            this.trackedLists.Clear();
        }

        private void Track(IPresetObject item)
        {
            if (item is INotifyPropertyChanged notifier)
            {
                notifier.PropertyChanged += this.Tracked_PropertyChanged;
                this.trackedItems.Add(notifier);
            }
        }

        private void TrackList(object list)
        {
            if (list is IBindingList bindingList)
            {
                bindingList.ListChanged += this.TrackedList_ListChanged;
                this.trackedLists.Add(bindingList);
            }
        }

        /// <summary>
        /// Rebuilds the flat row list (category headers followed by their presets when expanded) from a snapshot of the preset data.
        /// Works on copies only, so the source collections are never enumerated while they change.
        /// </summary>
        private void Rebuild()
        {
            this.rebuildPending = false;
            this.Untrack();

            MainViewModel viewModel = this.DataContext as MainViewModel;
            IEnumerable<IPresetObject> sourceItems = viewModel?.PresetsCategories;

            List<IPresetObject> newRows = new List<IPresetObject>();
            if (sourceItems != null)
            {
                this.TrackList(sourceItems);

                bool filtering = this.IsSearchActive;

                foreach (IPresetObject item in sourceItems.ToList())
                {
                    this.Track(item);

                    PresetDisplayCategory category = item as PresetDisplayCategory;
                    if (category != null && category.Presets != null)
                    {
                        this.TrackList(category.Presets);

                        List<Preset> visible = new List<Preset>();
                        foreach (Preset preset in category.Presets.ToList())
                        {
                            this.Track(preset);
                            if (filtering ? this.MatchesSearch(preset) : category.IsExpanded)
                            {
                                visible.Add(preset);
                            }
                        }

                        if (!filtering || visible.Count > 0)
                        {
                            newRows.Add(item);
                            newRows.AddRange(visible);
                        }
                    }
                    else if (!filtering)
                    {
                        newRows.Add(item);
                    }
                }
            }

            this.isSyncing = true;
            try
            {
                for (int i = 0; i < newRows.Count; i++)
                {
                    IPresetObject wanted = newRows[i];
                    if (i < this.rows.Count && ReferenceEquals(this.rows[i], wanted))
                    {
                        continue;
                    }

                    int existing = -1;
                    for (int j = i + 1; j < this.rows.Count; j++)
                    {
                        if (ReferenceEquals(this.rows[j], wanted))
                        {
                            existing = j;
                            break;
                        }
                    }

                    if (existing >= 0)
                    {
                        this.rows.Move(existing, i);
                    }
                    else
                    {
                        this.rows.Insert(i, wanted);
                    }
                }

                while (this.rows.Count > newRows.Count)
                {
                    this.rows.RemoveAt(this.rows.Count - 1);
                }

                this.SyncSelection();
            }
            finally
            {
                this.isSyncing = false;
            }
        }

        private void PresetListViewItem_OnLoaded(object sender, RoutedEventArgs e)
        {
            ListViewItem item = sender as ListViewItem;
            if (item != null)
            {
                item.Focusable = !(item.DataContext is PresetDisplayCategory);
            }
        }

        private void PresetListView_OnSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.isSyncing)
            {
                return;
            }

            object selected = this.presetListView.SelectedItem;
            if (selected is PresetDisplayCategory)
            {
                this.isSyncing = true;
                try
                {
                    this.presetListView.SelectedItem = e.RemovedItems.OfType<Preset>().FirstOrDefault();
                }
                finally
                {
                    this.isSyncing = false;
                }

                return;
            }

            Preset preset = selected as Preset;
            MainViewModel viewModel = this.DataContext as MainViewModel;
            if (preset != null && viewModel != null)
            {
                viewModel.SelectedPreset = preset;
            }
        }

        private void PresetListView_OnPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            ListViewItem listViewItem = VisualUpwardSearch(e.OriginalSource as DependencyObject);
            PresetDisplayCategory category = listViewItem?.DataContext as PresetDisplayCategory;
            if (category != null)
            {
                category.IsExpanded = !category.IsExpanded;
                e.Handled = true;
            }
        }

        private void PresetListView_OnPreviewMouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            ListViewItem listViewItem = VisualUpwardSearch(e.OriginalSource as DependencyObject);

            if (listViewItem != null)
            {
                PresetDisplayCategory category = listViewItem.DataContext as PresetDisplayCategory;
                if (category == null)
                {
                    listViewItem.IsSelected = true;
                    listViewItem.Focus();
                }

                this.moveTop.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveBottom.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveUp.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveDown.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveSplitter.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;

                this.moveSplitter1.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.moveSplitter2.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.setDefault.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.editPresetMenuItem.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.clonePresetMenuItem.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
                this.deletePresetMenuItem.Visibility = category == null ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        private static ListViewItem VisualUpwardSearch(DependencyObject source)
        {
            while (source != null && !(source is ListViewItem))
            {
                source = source is Visual || source is System.Windows.Media.Media3D.Visual3D ? VisualTreeHelper.GetParent(source) : LogicalTreeHelper.GetParent(source);
            }

            return source as ListViewItem;
        }

        private void Delete_OnClick(object sender, RoutedEventArgs e)
        {
            Preset preset = this.presetListView.SelectedItem as Preset;
            if (preset != null)
            {
                ((MainViewModel)this.DataContext).PresetRemove(preset);
            }
        }

        private void Delete_Execute(object sender, ExecutedRoutedEventArgs e)
        {
            Preset preset = this.presetListView.SelectedItem as Preset;
            if (preset != null)
            {
                ((MainViewModel)this.DataContext).PresetRemove(preset);
            }
        }

        private void ContextMenu_OnOpened(object sender, RoutedEventArgs e)
        {
            Preset preset = this.presetListView.SelectedItem as Preset;

            if (preset == null || preset.IsPresetDisabled || preset.IsBuildIn)
            {
                this.editPresetMenuItem.IsEnabled = false;
            }
            else
            {
                this.editPresetMenuItem.IsEnabled = true;
            }

            bool allowMove = !this.IsSearchActive;
            this.moveUp.IsEnabled = allowMove;
            this.moveDown.IsEnabled = allowMove;
            this.moveTop.IsEnabled = allowMove;
            this.moveBottom.IsEnabled = allowMove;
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


        private void PresetExpandAll_OnClick(object sender, RoutedEventArgs e)
        {
            this.SetAllCategoriesExpanded(true);
        }

        private void PresetCollapseAll_OnClick(object sender, RoutedEventArgs e)
        {
            this.SetAllCategoriesExpanded(false);
        }

        private void SetAllCategoriesExpanded(bool isExpanded)
        {
            IEnumerable<IPresetObject> sourceItems = (this.DataContext as MainViewModel)?.PresetsCategories;
            if (sourceItems == null)
            {
                return;
            }

            foreach (PresetDisplayCategory category in sourceItems.OfType<PresetDisplayCategory>().ToList())
            {
                category.IsExpanded = isExpanded;
            }
        }
    }
}
