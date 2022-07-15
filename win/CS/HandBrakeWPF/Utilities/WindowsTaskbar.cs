// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Win7.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using HandBrakeWPF.Helpers;
    using System.Windows.Shell;

    public class WindowsTaskbar
    {
        private static TaskbarItemInfo wintaskbar;

        public WindowsTaskbar()
        {
            wintaskbar ??= new TaskbarItemInfo();
        }

        public static TaskbarItemInfo GetTaskBar()
        {
            return wintaskbar;
        }
        
        public void SetTaskBarProgress(int percentage)
        {
            // Update the taskbar progress indicator.  The normal state shows a green progress bar.
            ThreadHelper.OnUIThread(
                () =>
                {
                    wintaskbar.ProgressState = TaskbarItemProgressState.Normal;
                    wintaskbar.ProgressValue = (double)percentage / 100;
                });
        }

        public void SetPaused()
        {
            if (wintaskbar.ProgressState != TaskbarItemProgressState.None)
            {
                ThreadHelper.OnUIThread(() => wintaskbar.ProgressState = TaskbarItemProgressState.Paused);
            }
        }

        public void SetNoProgress()
        {
            if (wintaskbar.ProgressState != TaskbarItemProgressState.None)
            {
                ThreadHelper.OnUIThread(() => wintaskbar.ProgressState = TaskbarItemProgressState.None);
            }
        }
    }
}