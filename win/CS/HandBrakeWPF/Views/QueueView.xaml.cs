// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;
    using System.ComponentModel;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.ViewModels;

    public partial class QueueView : Window
    {
        private QueueTask mouseActiveQueueTask;

        public QueueView()
        {
            this.InitializeComponent();
            this.SizeChanged += this.QueueView_SizeChanged;

            this.InputBindings.Add(new InputBinding(new CloseWindowCommand(this), new KeyGesture(Key.W, ModifierKeys.Control))); // Close Window

            this.InputBindings.Add(new InputBinding(new ProcessQueueShortcutCommand(new KeyGesture(Key.P, ModifierKeys.Control)), new KeyGesture(Key.P, ModifierKeys.Control))); // Play File
            this.InputBindings.Add(new InputBinding(new ProcessQueueShortcutCommand(new KeyGesture(Key.I, ModifierKeys.Control)), new KeyGesture(Key.I, ModifierKeys.Control))); // Source Dir
            this.InputBindings.Add(new InputBinding(new ProcessQueueShortcutCommand(new KeyGesture(Key.D, ModifierKeys.Control)), new KeyGesture(Key.D, ModifierKeys.Control))); // Destination Dir
            this.InputBindings.Add(new InputBinding(new ProcessQueueShortcutCommand(new KeyGesture(Key.E, ModifierKeys.Control)), new KeyGesture(Key.E, ModifierKeys.Control))); // Edit Job
            this.InputBindings.Add(new InputBinding(new ProcessQueueShortcutCommand(new KeyGesture(Key.R, ModifierKeys.Control)), new KeyGesture(Key.R, ModifierKeys.Control))); // Refresh
            this.InputBindings.Add(new InputBinding(new ProcessQueueShortcutCommand(new KeyGesture(Key.T, ModifierKeys.Control)), new KeyGesture(Key.T, ModifierKeys.Control))); // Move Up to Top
            this.InputBindings.Add(new InputBinding(new ProcessQueueShortcutCommand(new KeyGesture(Key.B, ModifierKeys.Control)), new KeyGesture(Key.B, ModifierKeys.Control))); // Move to Bottom


            this.DataContextChanged += this.QueueView_DataContextChanged;
        }

        private void QueueView_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            ((QueueViewModel)this.DataContext).SimpleViewChanged -= this.QueueView_SimpleViewChanged;
            ((QueueViewModel)this.DataContext).SimpleViewChanged += this.QueueView_SimpleViewChanged;
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }

        private void QueueView_SimpleViewChanged(object sender, EventArgs e)
        {
            if (!((QueueViewModel)this.DataContext).IsSimpleView)
            {
                if (Width < 900)
                {
                    this.Width = 900;
                    this.MinWidth = 900;
                }
            }
            else
            {
                this.MinWidth = 400;
            }
        }

        private void QueueView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            // Make the view adaptive. 
            if (e.WidthChanged)
            {
                if (((QueueViewModel)this.DataContext).IsSimpleView)
                {
                    this.MinWidth = 400;
                }
                else
                {
                    this.MinWidth = 900;
                }
            }
        }

        private void ContextMenu_OnOpened(object sender, RoutedEventArgs e)
        {
            ContextMenu menu = sender as ContextMenu;
            this.mouseActiveQueueTask = null;

            if (menu != null)
            {
                Point p = Mouse.GetPosition(this);
                IInputElement result = this.InputHitTest(p);

                if (result != null)
                {
                    ListBoxItem listBoxItem = FindParent2<ListBoxItem>(result);
                    if (listBoxItem != null)
                    {
                        this.mouseActiveQueueTask = listBoxItem.DataContext as QueueTask;
                    }
                }
            }

            // Handle menu state
            this.ResetMenuItem.Header = this.queueJobs.SelectedItems.Count > 1 ? Properties.Resources.QueueView_ResetSelectedJobs : Properties.Resources.QueueView_Reset;
            if (this.queueJobs.SelectedItems.Count > 1)
            {
                this.ResetMenuItem.IsEnabled = false;

                foreach (QueueTask task in this.queueJobs.SelectedItems)
                {
                    if (task.Status == QueueItemStatus.Error || task.Status == QueueItemStatus.Cancelled || task.Status == QueueItemStatus.Completed)
                    {
                        this.ResetMenuItem.IsEnabled = true;
                        break;
                    }
                }
            }
            else
            {
                var activeQueueTask = this.mouseActiveQueueTask;
                if (activeQueueTask != null && (activeQueueTask.Status == QueueItemStatus.Error || activeQueueTask.Status == QueueItemStatus.Cancelled || activeQueueTask.Status == QueueItemStatus.Completed))
                {
                    this.ResetMenuItem.IsEnabled = true;
                }
                else
                {
                    this.ResetMenuItem.IsEnabled = false;
                }
            }

            this.DeleteMenuItem.Header = this.queueJobs.SelectedItems.Count > 1 ? Properties.Resources.QueueView_DeleteSelected : Properties.Resources.QueueView_Delete;
            this.DeleteMenuItem.IsEnabled = (this.mouseActiveQueueTask != null || this.queueJobs.SelectedItems.Count >= 1) && (this.mouseActiveQueueTask?.TaskType != QueueTaskType.Breakpoint);
            this.EditMenuItem.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count == 1;
            this.openSourceDir.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count == 1;
            this.openDestDir.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count == 1;
            this.moveToBottomMenuItem.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count >= 1;
            this.moveToTopMenuItem.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count >= 1;
        }

        private static T FindParent2<T>(IInputElement from) where T : class
        {
            FrameworkElement fromElement = from as FrameworkElement;
            DependencyObject parent = fromElement?.Parent;

            if (parent == null)
            {
                parent = fromElement?.TemplatedParent;
            }
            
            T result = null;
            if (parent is T)
            {
                result = parent as T;
            }
            else if (parent != null)
            {
                result = FindParent<T>(parent);
            }

            return result;
        }

        private static T FindParent<T>(DependencyObject from) where T : class
        {
            DependencyObject parent = VisualTreeHelper.GetParent(from);

            T result = null;
            if (parent is T)
            {
                result = parent as T;
            }
            else if (parent != null)
            {
                result = FindParent<T>(parent);
            }

            return result;
        }

        private void OpenSourceDir_OnClick(object sender, RoutedEventArgs e)
        {
            ((QueueViewModel)this.DataContext).OpenSourceDirectory(this.mouseActiveQueueTask);
        }

        private void OpenDestDir_OnClick(object sender, RoutedEventArgs e)
        {
            ((QueueViewModel)this.DataContext).OpenDestinationDirectory(this.mouseActiveQueueTask);
        }

        private void QueueOptionsDropButton_OnClick(object sender, RoutedEventArgs e)
        {
            var button = sender as FrameworkElement;
            if (button != null && button.ContextMenu != null)
            {
                button.ContextMenu.PlacementTarget = button;
                button.ContextMenu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
                button.ContextMenu.IsOpen = true;
            }
        }

        private void QueueItem_Delete(object sender, RoutedEventArgs e)
        {
            if (this.queueJobs.SelectedItems.Count > 1)
            {
                ((QueueViewModel)this.DataContext).RemoveSelectedJobs();
            }
            else
            {
                ((QueueViewModel)this.DataContext).RemoveJob(this.mouseActiveQueueTask);
            }
        }

        private void QueueItem_Retry(object sender, RoutedEventArgs e)
        {
            if (this.queueJobs.SelectedItems.Count > 1)
            {
                ((QueueViewModel)this.DataContext).ResetSelectedJobs();
            }
            else
            {
                ((QueueViewModel)this.DataContext).RetryJob(this.mouseActiveQueueTask);
            }
        }

        private void QueueItem_Edit(object sender, RoutedEventArgs e)
        {
            ((QueueViewModel)this.DataContext).EditJob(this.mouseActiveQueueTask);
        }

        private void QueueDeleteJob_OnClick(object sender, RoutedEventArgs e)
        {
            Button button = sender as Button;
            QueueTask task = button?.DataContext as QueueTask;
            if (task != null)
            {
                ((QueueViewModel)this.DataContext).RemoveJob(task);
            }
        }

        private void DeleteItem_OnClick(object sender, ExecutedRoutedEventArgs e)
        {
            ((QueueViewModel)this.DataContext).RemoveSelectedJobs();
        }

        private void OptionsMenu_OnSubmenuOpened(object sender, RoutedEventArgs e)
        {
            if (((QueueViewModel)this.DataContext).ExtendedQueueDisplay)
            {
                this.extendedQueueDisplay.Header = Properties.Resources.QueueView_MinimalQueueDisplay;
            }
            else
            {
                this.extendedQueueDisplay.Header = Properties.Resources.QueueView_ExtendedQueueDisplay;
            }

            if (((QueueViewModel)this.DataContext).IsSimpleView)
            {
                this.simpleQueueDisplay.Header = Properties.Resources.QueueView_AdvancedQueueDisplay;
            }
            else
            {
                this.simpleQueueDisplay.Header = Properties.Resources.QueueView_SimpleQueueDisplay;
            }
        }

        private void QueueView_OnClosing(object sender, CancelEventArgs e)
        {
            ((QueueViewModel)this.DataContext).BackupQueue();
        }
    }
}
