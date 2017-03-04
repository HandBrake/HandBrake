﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ProcessShortcutCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Keyboard Shortcut Processor
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows;
    using System.Windows.Input;

    using Caliburn.Micro;

    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Keyboard Shortcut Processor
    /// </summary>
    public class ProcessShortcutCommand : ICommand
    {
        /// <summary>
        /// The Gesture
        /// </summary>
        private readonly KeyGesture gesture;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessShortcutCommand"/> class.
        /// </summary>
        /// <param name="gesture">
        /// The gesture.
        /// </param>
        public ProcessShortcutCommand(KeyGesture gesture)
        {
            this.gesture = gesture;
        }

        /// <summary>
        /// Defines the method to be called when the command is invoked.
        /// </summary>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public void Execute(object parameter)
        {
            if (gesture != null)
            {
                IMainViewModel mainViewModel = IoC.Get<IMainViewModel>();
                
                // Start Encode (Ctrl+S)
                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.S)
                {
                    mainViewModel.StartEncode();
                }

                // Stop Encode (Ctrl+K)
                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.K)
                {
                    mainViewModel.StopEncode();
                }

                // Open Log Window (Ctrl+L)
                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.L)
                {
                    mainViewModel.OpenLogWindow();
                }

                // Open Queue Window (Ctrl+Q)
                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.Q)
                {
                    mainViewModel.OpenQueueWindow();
                }

                // Add to Queue (Ctrl+A)
                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.A)
                {
                    mainViewModel.AddToQueue();
                }

                // Scan a File (Alt+O)
                if (gesture.Modifiers == ModifierKeys.Alt && gesture.Key == Key.O)
                {
                    mainViewModel.SelectSourceWindow();
                }

                // Scan a File (Ctrl+O)
                if (gesture.Modifiers == ModifierKeys.Control && gesture.Key == Key.O)
                {
                    mainViewModel.FileScan();
                }

                // Scan a Folder (Ctrl+Shift+O)
                if (gesture.Modifiers == (ModifierKeys.Control | ModifierKeys.Shift) && gesture.Key == Key.O)
                {
                    mainViewModel.FolderScan();
                }

                // Launch Help (F1)
                if (gesture.Key == Key.F1)
                {
                    mainViewModel.LaunchHelp();
                }

                if (gesture.Modifiers == (ModifierKeys.Control | ModifierKeys.Shift) && gesture.Key == Key.G)
                {
                    GC.Collect();
                    MessageBox.Show("DEBUG: Garbage Collection Completed");
                }
            }
        }

        /// <summary>
        /// Defines the method that determines whether the command can execute in its current state.
        /// </summary>
        /// <returns>
        /// true if this command can be executed; otherwise, false.
        /// </returns>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public bool CanExecute(object parameter)
        {
            return true;
        }

        /// <summary>
        /// Can Execute Changed
        /// </summary>
        public event EventHandler CanExecuteChanged;
    }
}
