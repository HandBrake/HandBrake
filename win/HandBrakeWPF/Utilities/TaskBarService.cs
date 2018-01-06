// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TaskBarService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A class implimenting TaskBar Progress for Windows 7 and up.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.Windows.Shell;
    using HandBrake.Utilities.Interfaces;

    public class TaskBarService : ITaskBarService
    {
        public TaskBarService()
        {
            if (WindowsTaskbar == null)
                WindowsTaskbar = new TaskbarItemInfo();
        }

        /// <summary>
        /// The Windows Taskbar
        /// </summary>
        public static TaskbarItemInfo WindowsTaskbar;

        public void DisableTaskBarProgress()
        {
            Caliburn.Micro.Execute.OnUIThread(() => WindowsTaskbar.ProgressState = TaskbarItemProgressState.None);
        }

        public void SetTaskBarProgress(int percentage)
        {
            // Update the taskbar progress indicator.  The normal state shows a green progress bar.
            Caliburn.Micro.Execute.OnUIThread(
                () =>
                {
                    WindowsTaskbar.ProgressState = TaskbarItemProgressState.Normal;
                    WindowsTaskbar.ProgressValue = (double)percentage / 100;
                });
        }
    }
}