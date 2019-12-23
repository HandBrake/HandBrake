// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueTwoView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for QueueTwoView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Linq;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for VideoView
    /// </summary>
    public partial class QueueView : Window
    {
        private QueueTask mouseActiveQueueTask;

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueView"/> class.
        /// </summary>
        public QueueView()
        {
            this.InitializeComponent();
            this.SizeChanged += this.QueueView_SizeChanged;
        }

        private void QueueView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            // Make the view adaptive. 
            if (e.WidthChanged)
            {
                int queueSizeLimit = 745;

                this.summaryTabControl.Visibility = this.ActualWidth < queueSizeLimit ? Visibility.Collapsed : Visibility.Visible;
                this.leftTabPanel.Width = this.ActualWidth < queueSizeLimit ? new GridLength(this.ActualWidth - 10, GridUnitType.Star) : new GridLength(3, GridUnitType.Star);
                this.leftTabPanel.MaxWidth = this.ActualWidth < queueSizeLimit ? 750 : 650;
            }
        }

        private void ContextMenu_OnOpened(object sender, RoutedEventArgs e)
        {
            ContextMenu menu = sender as ContextMenu;
            this.mouseActiveQueueTask = null;

            if (menu != null)
            {
                Point p = Mouse.GetPosition(this);
                HitTestResult result = VisualTreeHelper.HitTest(this, p);

                if (result != null)
                {
                    ListBoxItem listBoxItem = FindParent<ListBoxItem>(result.VisualHit);
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
                    if (task.Status == QueueItemStatus.Error || task.Status == QueueItemStatus.Completed)
                    {
                        this.ResetMenuItem.IsEnabled = true;
                        break;
                    }
                }
            }
            else
            {
                var activeQueueTask = this.mouseActiveQueueTask;
                if (activeQueueTask != null && (activeQueueTask.Status == QueueItemStatus.Error || activeQueueTask.Status == QueueItemStatus.Completed))
                {
                    this.ResetMenuItem.IsEnabled = true;
                }
                else
                {
                    this.ResetMenuItem.IsEnabled = false;
                }
            }

            this.DeleteMenuItem.Header = this.queueJobs.SelectedItems.Count > 1 ? Properties.Resources.QueueView_DeleteSelected : Properties.Resources.QueueView_Delete;
            this.DeleteMenuItem.IsEnabled = this.mouseActiveQueueTask != null || this.queueJobs.SelectedItems.Count >= 1;

            this.EditMenuItem.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count == 1;
            this.openSourceDir.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count == 1;
            this.openDestDir.IsEnabled = this.mouseActiveQueueTask != null && this.queueJobs.SelectedItems.Count == 1;
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
    }
}
