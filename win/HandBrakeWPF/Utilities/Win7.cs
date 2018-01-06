// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Win7.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A class implimenting Windows 7 Specific features
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.Windows.Shell;

    /// <summary>
    /// A class implementing Windows 7 Specific features
    /// </summary>
    public class Win7
    {
        /// <summary>
        /// The Windows Taskbar
        /// </summary>
        public static TaskbarItemInfo WindowsTaskbar;

        /// <summary>
        /// Initializes a new instance of the <see cref="Win7"/> class.
        /// </summary>
        public Win7()
        {
            if (WindowsTaskbar == null)
                WindowsTaskbar = new TaskbarItemInfo();
        }

        /// <summary>
        /// Gets a value indicating whether is windows seven.
        /// </summary>
        public bool IsWindowsSeven
        {
            get
            {
                return true;
            }
        }

        /// <summary>
        /// The get task bar.
        /// </summary>
        /// <returns>
        /// The <see cref="TaskbarItemInfo"/>.
        /// </returns>
        public TaskbarItemInfo GetTaskBar()
        {
            return WindowsTaskbar;
        }

        /// <summary>
        /// Set the Task Bar Percentage.
        /// </summary>
        /// <param name="percentage">
        /// The percentage.
        /// </param>
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

        /// <summary>
        /// Disable Task Bar Progress
        /// </summary>
        public void SetTaskBarProgressToNoProgress()
        {
            Caliburn.Micro.Execute.OnUIThread(() => WindowsTaskbar.ProgressState = TaskbarItemProgressState.None);
        }
    }
}