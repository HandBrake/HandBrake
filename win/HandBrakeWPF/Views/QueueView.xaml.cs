// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for VideoView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Views
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    using HandBrake.Services.Queue.Model;
    using HandBrake.ViewModels;

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

            this.openSourceDir.IsEnabled = this.mouseActiveQueueTask != null;
            this.openDestDir.IsEnabled = this.mouseActiveQueueTask != null;
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
    }
}
