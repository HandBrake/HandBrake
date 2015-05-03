// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CancelScanCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Command to cancel a scan that is in progress
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Windows.Input;

    using HandBrake.ApplicationServices.Services.Scan.EventArgs;
    using HandBrake.ApplicationServices.Services.Scan.Interfaces;

    /// <summary>
    /// Command to cancel a scan that is in progress
    /// </summary>
    public class CancelScanCommand : ICommand
    {
        /// <summary>
        /// The scan service wrapper.
        /// </summary>
        private readonly IScan scanServiceWrapper;

        /// <summary>
        /// Initializes a new instance of the <see cref="CancelScanCommand"/> class.
        /// </summary>
        /// <param name="ssw">
        /// The scan service wrapper.
        /// </param>
        public CancelScanCommand(IScan ssw)
        {
            this.scanServiceWrapper = ssw;
            this.scanServiceWrapper.ScanStarted += this.ScanServiceWrapperScanStared;
            this.scanServiceWrapper.ScanCompleted += this.ScanServiceWrapperScanCompleted;
        }

        /// <summary>
        /// The scan service Scan Completed Event Handler.
        /// Fires CanExecuteChanged
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The ScanCompletedEventArgs.
        /// </param>
        private void ScanServiceWrapperScanCompleted(object sender, ScanCompletedEventArgs e)
        {
            Caliburn.Micro.Execute.OnUIThread(this.OnCanExecuteChanged);    
        }

        /// <summary>
        /// The scan service scan started event handler.
        /// Fires CanExecuteChanged
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void ScanServiceWrapperScanStared(object sender, EventArgs e)
        {
            Caliburn.Micro.Execute.OnUIThread(this.OnCanExecuteChanged);    
        }

        #region Implementation of ICommand

        /// <summary>
        /// Defines the method to be called when the command is invoked.
        /// </summary>
        /// <param name="parameter">Data used by the command.  If the command does not require data to be passed, this object can be set to null.</param>
        public void Execute(object parameter)
        {
            this.scanServiceWrapper.Stop();
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
            if (this.scanServiceWrapper != null)
            {
                return this.scanServiceWrapper.IsScanning;
            }

            return false;
        }

        /// <summary>
        /// The can execute changed.
        /// </summary>
        public event EventHandler CanExecuteChanged;

        /// <summary>
        /// The on can execute changed.
        /// </summary>
        protected virtual void OnCanExecuteChanged()
        {
            EventHandler handler = this.CanExecuteChanged;
            if (handler != null)
            {
                handler(this, EventArgs.Empty);
            }
        }

        #endregion
    }
}
