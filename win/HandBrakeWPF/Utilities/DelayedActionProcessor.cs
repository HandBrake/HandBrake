// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DelayedActionProcessor.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Action processor that supports queueing/delayed action processing.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
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
            if (timer != null)
            {
                timer.Stop();
                timer.Close();                
            }

            timer = new Timer(timems) { AutoReset = true };
            timer.Elapsed += this.timer_Elapsed;
            task = taskReset;
            timer.Stop();
            timer.Start();
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
            if (task != null)
            {
                timer.Stop();
                task();      
            }
        }
    }
}
