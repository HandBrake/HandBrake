// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DriveDetectService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Drive Detection Helper.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.Management;
    using System.Threading;

    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// Drive Detection Helper.
    /// </summary>
    public class DriveDetectService : IDriveDetectService
    {
        /// <summary>
        /// The watcher.
        /// </summary>
        private ManagementEventWatcher watcher;

        /// <summary>
        /// The detection action.
        /// </summary>
        private Action detectionAction;

        /// <summary>
        /// The start detection.
        /// </summary>
        /// <param name="action">
        /// The detection Action.
        /// </param>
        public void StartDetection(Action action)
        {
            ThreadPool.QueueUserWorkItem(
                delegate
                {
                    this.detectionAction = action;

                    var options = new ConnectionOptions { EnablePrivileges = true };
                    var scope = new ManagementScope(@"root\CIMV2", options);

                    try
                    {
                        var query = new WqlEventQuery
                        {
                            EventClassName = "__InstanceModificationEvent",
                            WithinInterval = TimeSpan.FromSeconds(1),
                            Condition = @"TargetInstance ISA 'Win32_LogicalDisk' and TargetInstance.DriveType = 5" // DriveType - 5: CDROM
                        };

                        this.watcher = new ManagementEventWatcher(scope, query);
                        this.watcher.EventArrived += this.WatcherEventArrived;
                        this.watcher.Start();
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine(e.Message);
                    }
                });
        }

        /// <summary>
        /// The close.
        /// </summary>
        public void Close()
        {
            if (watcher != null)
            {
                this.watcher.Stop();
            }
        }

        /// <summary>
        /// The watcher_ event arrived.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArrivedEventArgs.
        /// </param>
        private void WatcherEventArrived(object sender, EventArrivedEventArgs e)
        {
            if (this.detectionAction != null)
            {
                this.detectionAction();
            }
        }
    }
}
