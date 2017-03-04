// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Loading.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for Loading.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Threading;

    /// <summary>
    /// Interaction logic for Loading.xaml
    /// </summary>
    public partial class Loading : UserControl
    {
        #region Fields

        /// <summary>
        /// Step
        /// </summary>
        private const double Step = Math.PI * 2 / 10.0;

        /// <summary>
        /// Timer for the spinning effect.
        /// </summary>
        private readonly DispatcherTimer timer;

        #endregion

        #region Constructors

        /// <summary>
        /// Initializes a new instance of the <see cref="Loading"/> class.
        /// </summary>
        public Loading()
        {
            InitializeComponent();

            IsVisibleChanged += OnVisibleChanged;

            this.timer = new DispatcherTimer(DispatcherPriority.ContextIdle, Dispatcher)
            {
                Interval = new TimeSpan(0, 0, 0, 0, 75)
            };
        }

        #endregion

        /// <summary>
        /// Sets the position.
        /// </summary>
        /// <param name="ellipse">The ellipse.</param>
        /// <param name="offset">The offset.</param>
        /// <param name="posOffSet">The pos off set.</param>
        /// <param name="step">The step to change.</param>
        private static void SetPosition(DependencyObject ellipse, double offset, double posOffSet, double step)
        {
            ellipse.SetValue(Canvas.LeftProperty, 50 + (Math.Sin(offset + (posOffSet * step)) * 50));
            ellipse.SetValue(Canvas.TopProperty, 50 + (Math.Cos(offset + (posOffSet * step)) * 50));
        }

        /// <summary>
        /// Starts this instance.
        /// </summary>
        private void Start()
        {
            this.timer.Tick += this.OnTick;
            this.timer.Start();
        }

        /// <summary>
        /// Stops this instance.
        /// </summary>
        private void Stop()
        {
            this.timer.Stop();
            this.timer.Tick -= this.OnTick;
        }

        /// <summary>
        /// Run the Animation
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void OnTick(object sender, EventArgs e)
        {
            this.rotate.Angle = (this.rotate.Angle + 36) % 360;
        }

        /// <summary>
        /// Start and Stop the effect when the visibility changes
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void OnVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if ((bool)e.NewValue)
            {
                Start();
            }
            else
            {
                Stop();
            }
        }

        /// <summary>
        /// Setup the Canvas
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void CanvasLoaded(object sender, RoutedEventArgs e)
        {
            SetPosition(this.circle0, Math.PI, 0.0, Step);
            SetPosition(this.circle1, Math.PI, 1.0, Step);
            SetPosition(this.circle2, Math.PI, 2.0, Step);
            SetPosition(this.circle3, Math.PI, 3.0, Step);
            SetPosition(this.circle4, Math.PI, 4.0, Step);
            SetPosition(this.circle5, Math.PI, 5.0, Step);
            SetPosition(this.circle6, Math.PI, 6.0, Step);
            SetPosition(this.circle7, Math.PI, 7.0, Step);
            SetPosition(this.circle8, Math.PI, 8.0, Step);
        }

        /// <summary>
        /// Stop when we unload this canvas
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void CanvasUnloaded(object sender, RoutedEventArgs e)
        {
            Stop();
        }
    }
}
