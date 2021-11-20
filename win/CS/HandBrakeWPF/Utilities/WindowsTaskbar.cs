// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Win7.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
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
            Caliburn.Micro.Execute.OnUIThread(
                () =>
                {
                    wintaskbar.ProgressState = TaskbarItemProgressState.Normal;
                    wintaskbar.ProgressValue = (double)percentage / 100;
                });
        }

        public void SetTaskBarProgressToNoProgress()
        {
            if (wintaskbar.ProgressState != TaskbarItemProgressState.None)
            {
                Caliburn.Micro.Execute.OnUIThread(() => wintaskbar.ProgressState = TaskbarItemProgressState.None);
            }
        }
    }
}