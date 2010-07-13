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
            if (IsWindowsSeven)
            {
                windowsTaskbar = TaskbarManager.Instance;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this is Windows Seven.
        /// </summary>
        public bool IsWindowsSeven
        {
            get
            {
                OperatingSystem os = Environment.OSVersion;
                return os.Version.Major >= 6 && os.Version.Minor >= 1;
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
            if (!IsWindowsSeven)
            {
                return;
            }
            windowsTaskbar.SetProgressState(TaskbarProgressBarState.Normal); // todo CHECK THIS
            windowsTaskbar.SetProgressValue(percentage, 100);
        }

        /// <summary>
        /// Disable Task Bar Progress
        /// </summary>
        public void SetTaskBarProgressToNoProgress()
        {
            if (!IsWindowsSeven)
            {
                return;
            }

            windowsTaskbar.SetProgressState(TaskbarProgressBarState.NoProgress);
        }
    }
}