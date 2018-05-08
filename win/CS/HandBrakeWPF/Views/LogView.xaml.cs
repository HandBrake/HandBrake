// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for LogView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;
    using System.Diagnostics;
    using System.Windows;
    using System.Windows.Controls;

    using HandBrakeWPF.Services.Logging.EventArgs;
    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for LogView.xaml
    /// </summary>
    public partial class LogView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LogView"/> class.
        /// </summary>
        public LogView()
        {
            this.InitializeComponent();
            this.DataContextChanged += this.LogView_DataContextChanged;
        }

        /// <summary>
        /// The log view_ data context changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void LogView_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            LogViewModel vm = e.NewValue as LogViewModel;
            if (vm != null)
            {
                this.logText.AppendText(vm.ActivityLog);
                vm.LogMessageReceived += this.Vm_LogMessageReceived;
            }
        }

        /// <summary>
        /// The vm_ log message received.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void Vm_LogMessageReceived(object sender, LogEventArgs e)
        {
            try
            {
                if (e == null)
                {
                    Caliburn.Micro.Execute.OnUIThread(
                        () =>
                            {
                                LogViewModel vm = this.DataContext as LogViewModel;
                                if (vm != null)
                                {
                                    this.logText.Clear();
                                    this.logText.AppendText(vm.ActivityLog);
                                }
                                else
                                {
                                    Debug.WriteLine("Failed to Reset Log correctly.");
                                }
                            });   
                }
                else
                {
                    // This works better than Data Binding because of the scroll.
                    this.logText.AppendText(Environment.NewLine + e.Log.Content);

                    if (this.AutoScroll.IsChecked)
                    {
                        this.logText.ScrollToEnd();
                    }
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }
        }

        /// <summary>
        /// Hide the Toolbar Endplate.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ToolBarLoaded(object sender, RoutedEventArgs e)
        {
            ToolBar toolBar = sender as ToolBar;
            if (toolBar != null)
            {
                var overflowGrid = toolBar.Template.FindName("OverflowGrid", toolBar) as FrameworkElement;
                if (overflowGrid != null)
                {
                    overflowGrid.Visibility = Visibility.Collapsed;
                }
            }
        }
    }
}
