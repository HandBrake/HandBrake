// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RefireControl.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The refire control.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Threading;
    using System.Windows;

    /// <summary>
    /// The refire control.
    /// </summary>
    public class RefireControl
    {
        #region Constants and Fields

        /// <summary>
        ///     How long stages last.
        /// </summary>
        private const int StageDurationMsec = 900;

        /// <summary>
        /// The delays. At each stage the refire rate increases.
        /// </summary>
        private static readonly List<int> Delays = new List<int> { 500, 200, 100, 50, 20 };

        /// <summary>
        /// The refire action.
        /// </summary>
        private readonly Action refireAction;

        /// <summary>
        /// The refire sync.
        /// </summary>
        private readonly object refireSync = new object();

        /// <summary>
        /// The refire timer.
        /// </summary>
        private Timer refireTimer;

        /// <summary>
        /// The running.
        /// </summary>
        private bool running;

        /// <summary>
        /// The stopwatch.
        /// </summary>
        private Stopwatch stopwatch;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="RefireControl"/> class.
        /// </summary>
        /// <param name="refireAction">
        /// The refire action.
        /// </param>
        public RefireControl(Action refireAction)
        {
            this.refireAction = refireAction;
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// The begin.
        /// </summary>
        public void Begin()
        {
            lock (this.refireSync)
            {
                this.stopwatch = Stopwatch.StartNew();
                this.running = true;

                // Fire once immediately.
                this.refireAction();

                this.refireTimer = new Timer(
                    obj =>
                        {
                            lock (this.refireSync)
                            {
                                if (this.running)
                                {
                                    var stage = (int)(this.stopwatch.ElapsedMilliseconds / StageDurationMsec);

                                    int newDelay = stage >= Delays.Count ? Delays[Delays.Count - 1] : Delays[stage];

                                    Application.Current.Dispatcher.BeginInvoke(this.refireAction);

                                    this.refireTimer.Change(newDelay, newDelay);
                                }
                            }
                        }, 
                    null, 
                    Delays[0], 
                    Delays[0]);
            }
        }

        /// <summary>
        /// The stop.
        /// </summary>
        public void Stop()
        {
            lock (this.refireSync)
            {
                this.stopwatch.Stop();
                this.running = false;

                if (this.refireTimer != null)
                {
                    this.refireTimer.Dispose();
                }
            }
        }

        #endregion
    }
}