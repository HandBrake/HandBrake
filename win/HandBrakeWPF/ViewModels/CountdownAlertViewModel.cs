// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CountdownAlertViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Countdown Alert View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.Windows.Threading;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    ///  The Countdown Alert View Model
    /// </summary>
    public class CountdownAlertViewModel : ViewModelBase, ICountdownAlertViewModel
    {
        #region Private Fields

        /// <summary>
        /// The countdown time.
        /// </summary>
        private const int CountdownTime = 60;

        /// <summary>
        /// The action.
        /// </summary>
        private string action;

        /// <summary>
        /// The timer.
        /// </summary>
        private DispatcherTimer timer;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="CountdownAlertViewModel"/> class.
        /// </summary>
        public CountdownAlertViewModel()
        {
            this.IsCancelled = false;

            Caliburn.Micro.Execute.OnUIThread(
                () =>
                {
                    timer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
                    timer.Tick += this.timer_Tick;

                    timer.Start();
                });
        }

        #endregion

        #region Public Properties

        /// <summary>
        /// Gets or sets a value indicating whether is cancelled.
        /// </summary>
        public bool IsCancelled { get; set; }

        /// <summary>
        /// Gets the notice message.
        /// </summary>
        public string NoticeMessage
        {
            get
            {
                return string.Format(Resources.CountdownAlertViewModel_NoticeMessage, action, CountdownTime - this.Ticks);
            }
        }

        /// <summary>
        /// Gets or sets the ticks.
        /// </summary>
        public int Ticks { get; set; }

        #endregion

        #region Public Methods and Operators

        /// <summary>
        /// The cancel.
        /// </summary>
        public void Cancel()
        {
            this.IsCancelled = true;
            timer.Stop();
            this.Ticks = 0;
            this.TryClose();
        }

        /// <summary>
        /// The proceed.
        /// </summary>
        public void Proceed()
        {
            this.IsCancelled = false;
            timer.Stop();
            this.Ticks = 0;
            this.TryClose();
        }

        /// <summary>
        /// The set action.
        /// </summary>
        /// <param name="actionMsg">
        /// The action.
        /// </param>
        public void SetAction(string actionMsg)
        {
            this.Ticks = 0;
            timer.Start();
            this.action = actionMsg;
        }

        #endregion

        #region Methods

        /// <summary>
        /// The timer_ tick.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void timer_Tick(object sender, EventArgs e)
        {
            this.Ticks = this.Ticks + 1;
            this.NotifyOfPropertyChange(() => this.NoticeMessage);
            if (this.Ticks > CountdownTime)
            {
                timer.Stop();
                this.Ticks = 0;
                this.TryClose();
            }
        }

        #endregion
    }
}