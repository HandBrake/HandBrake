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
    using System.Windows.Input;

    using HandBrake.App.Core.Utilities;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Logging.EventArgs;
    using HandBrakeWPF.ViewModels;

    public partial class LogView : Window
    {
        private DelayedActionProcessor delayProcessor = new DelayedActionProcessor();
        private LogViewModel viewModel;
        
        public LogView()
        {
            this.InitializeComponent();
            this.DataContextChanged += this.LogView_DataContextChanged;
            this.Closed += this.LogView_Closed;
            this.InputBindings.Add(new InputBinding(new CloseWindowCommand(this), new KeyGesture(Key.W, ModifierKeys.Control))); // Close Window
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }

        private void LogView_Closed(object sender, EventArgs e)
        {
            ((LogViewModel)this.DataContext).LogMessageReceived -= this.Vm_LogMessageReceived;
            ((LogViewModel)this.DataContext).LogResetEvent -= this.Vm_LogResetEvent;
        }

        private void LogView_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            LogViewModel vm = e.NewValue as LogViewModel;
            if (vm != null)
            {
                viewModel = vm;
                this.logText.AppendText(vm.ActivityLog);
                vm.LogMessageReceived += this.Vm_LogMessageReceived;
                vm.LogResetEvent += this.Vm_LogResetEvent;
            }
        }

        private void Vm_LogResetEvent(object sender, EventArgs e)
        {
            ThreadHelper.OnUIThread(
                () =>
                {
                    this.logText.Clear();
                    this.logText.AppendText(this.viewModel.ActivityLog.TrimStart());
                });
        }

        private void Vm_LogMessageReceived(object sender, LogEventArgs e)
        {
            try
            { 
                // This works better than Data Binding because of the scroll.
                this.logText.AppendText(e.Log.Content + Environment.NewLine);

                if (this.AutoScroll.IsChecked)
                {
                    delayProcessor.PerformTask(() => ThreadHelper.OnUIThread(() => this.logText.ScrollToEnd()), 100);
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc);
            }
        }

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
