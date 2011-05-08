/*  Win7.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using Microsoft.WindowsAPICodePack.Taskbar;

    /// <summary>
    /// A class implimenting Windows 7 Specific features
    /// </summary>
    public class Win7
    {
        /// <summary>
        /// The Windows Taskbar
        /// </summary>
        private TaskbarManager windowsTaskbar;

        /// <summary>
        /// Initializes a new instance of the <see cref="Win7"/> class.
        /// </summary>
        public Win7()
        {
            if (this.IsWindowsSeven)
            {
                this.windowsTaskbar = TaskbarManager.Instance;
                this.windowsTaskbar.ApplicationId = "HandBrake";
            }
        }

        /// <summary>
        /// Gets a value indicating whether this is Windows Seven.
        /// </summary>
        public bool IsWindowsSeven
        {
            get
            {
                return TaskbarManager.IsPlatformSupported;
            }
        }

        /// <summary>
        /// Set the Task Bar Percentage.
        /// </summary>
        /// <param name="percentage">
        /// The percentage.
        /// </param>
        public void SetTaskBarProgress(int percentage)
        {
            if (!this.IsWindowsSeven)
            {
                return;
            }

            this.windowsTaskbar.SetProgressState(TaskbarProgressBarState.Normal);
            this.windowsTaskbar.SetProgressValue(percentage, 100);
        }

        /// <summary>
        /// Disable Task Bar Progress
        /// </summary>
        public void SetTaskBarProgressToNoProgress()
        {
            if (!this.IsWindowsSeven)
            {
                return;
            }

            this.windowsTaskbar.SetProgressState(TaskbarProgressBarState.NoProgress);
        }
    }
}