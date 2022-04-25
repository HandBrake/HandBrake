// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DelayedActionProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Action processor that supports queueing/delayed action processing.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Utilities
{
    using System;
    using System.Timers;

    /// <summary>
    /// An Action processor that supports queueing/delayed action processing.
    /// </summary>
    public class DelayedActionProcessor
    {
        /// <summary>
        /// The task.
        /// </summary>
        private Action task;

        /// <summary>
        /// The timer.
        /// </summary>
        private Timer timer;

        /// <summary>
        /// The set task.
        /// </summary>
        /// <param name="taskReset">
        /// The task reset.
        /// </param>
        /// <param name="timems">
        /// The timems.
        /// </param>
        public void PerformTask(Action taskReset, int timems)
        {
            if (this.timer != null)
            {
                this.timer.Stop();
                this.timer.Close();                
            }

            this.timer = new Timer(timems) { AutoReset = true };
            this.timer.Elapsed += this.timer_Elapsed;
            this.task = taskReset;
            this.timer.Stop();
            this.timer.Start();
        }

        /// <summary>
        /// The timer_ elapsed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void timer_Elapsed(object sender, ElapsedEventArgs e)
        {
            if (this.task != null)
            {
                this.timer.Stop();
                this.task();      
            }
        }
    }
}
