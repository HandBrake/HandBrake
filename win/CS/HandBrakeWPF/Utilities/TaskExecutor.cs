// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TaskScheduler.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------
namespace HandBrakeWPF.Utilities
{
    using System;
    using System.Threading;

    public class TaskExecutor
    {
        private Timer timer;

        private Action action;

        public void ScheduleTask(int hour, int min, Action task)
        {
            if (timer != null)
            {
                this.CancelTask();
            }

            DateTime now = DateTime.Now; 
            DateTime firstRun = new DateTime(now.Year, now.Month, now.Day, hour, min, 0, 0);
            if (now > firstRun)
            {
                firstRun = firstRun.AddDays(1);
            }

            TimeSpan timeToGo = firstRun - now;
            if (timeToGo <= TimeSpan.Zero)
            {
                timeToGo = TimeSpan.Zero;
            }

            action = task;

            timer = new Timer(x =>
            {
                action.Invoke();
            }, null, timeToGo, TimeSpan.FromSeconds(60));


        }

        public void CancelTask()
        {
            if (this.timer != null)
            {
                this.timer.Dispose();
                this.timer = null;
            }
        }
    }
}
