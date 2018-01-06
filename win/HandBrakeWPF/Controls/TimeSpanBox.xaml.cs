// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TimeSpanBox.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for TimeSpanBox.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;
    using System.Windows.Media;

    /// <summary>
    ///     Interaction logic for TimeSpanBox.xaml
    /// </summary>
    public partial class TimeSpanBox
    {
        #region Constants and Fields

        /// <summary>
        /// The maximum property name.
        /// </summary>
        public const string MaximumPropertyName = "Maximum";

        /// <summary>
        /// The minimum property name.
        /// </summary>
        public const string MinimumPropertyName = "Minimum";

        /// <summary>
        /// The modulus property name.
        /// </summary>
        public const string ModulusPropertyName = "Modulus";

        /// <summary>
        /// The allow empty property.
        /// </summary>
        public static readonly DependencyProperty AllowEmptyProperty = DependencyProperty.Register(
            "AllowEmpty", typeof(bool), typeof(TimeSpanBox), new PropertyMetadata(true, OnAllowEmptyChanged));

        /// <summary>
        /// The maximum property.
        /// </summary>
        public static readonly DependencyProperty MaximumProperty = DependencyProperty.Register(
            MaximumPropertyName, typeof(int), typeof(TimeSpanBox), new UIPropertyMetadata(int.MaxValue));

        /// <summary>
        /// The minimum property.
        /// </summary>
        public static readonly DependencyProperty MinimumProperty = DependencyProperty.Register(
            MinimumPropertyName, typeof(int), typeof(TimeSpanBox), new UIPropertyMetadata(int.MinValue));

        /// <summary>
        /// The modulus property.
        /// </summary>
        public static readonly DependencyProperty ModulusProperty = DependencyProperty.Register(
            ModulusPropertyName, typeof(int), typeof(TimeSpanBox), new UIPropertyMetadata(0));

        /// <summary>
        /// The number property.
        /// </summary>
        public static readonly DependencyProperty NumberProperty = DependencyProperty.Register(
            "Number", typeof(int), typeof(TimeSpanBox), new PropertyMetadata(OnNumberChanged));

        /// <summary>
        /// The show time span property.
        /// </summary>
        public static readonly DependencyProperty ShowTimeSpanProperty = DependencyProperty.Register(
            "ShowTimeSpan", typeof(bool), typeof(TimeSpanBox), new PropertyMetadata(OnShowTimeSpanChanged));

        /// <summary>
        /// The select all threshold.
        /// </summary>
        private static readonly TimeSpan SelectAllThreshold = TimeSpan.FromMilliseconds(500);

        /// <summary>
        /// The has focus.
        /// </summary>
        private bool hasFocus;

        /// <summary>
        /// The last focus mouse down.
        /// </summary>
        private DateTime lastFocusMouseDown;

        /// <summary>
        /// The none caption.
        /// </summary>
        private string noneCaption;

        /// <summary>
        /// The refire control.
        /// </summary>
        private RefireControl refireControl;

        /// <summary>
        /// The suppress refresh.
        /// </summary>
        private bool suppressRefresh;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="TimeSpanBox"/> class. 
        /// </summary>
        public TimeSpanBox()
        {
            this.noneCaption = "(none)";
            this.UpdateBindingOnTextChange = true;
            this.ShowIncrementButtons = true;
            this.SelectAllOnClick = true;

            this.InitializeComponent();

            this.RefreshNumberBox();
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets a value indicating whether show time span.
        /// </summary>
        public bool ShowTimeSpan
        {
            get
            {
                return (bool)this.GetValue(ShowTimeSpanProperty);
            }

            set
            {
                this.SetValue(ShowTimeSpanProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether allow empty.
        /// </summary>
        public bool AllowEmpty
        {
            get
            {
                return (bool)this.GetValue(AllowEmptyProperty);
            }

            set
            {
                this.SetValue(AllowEmptyProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets the maximum.
        /// </summary>
        public int Maximum
        {
            get
            {
                return (int)this.GetValue(MaximumProperty);
            }
            set
            {
                this.SetValue(MaximumProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets the minimum.
        /// </summary>
        public int Minimum
        {
            get
            {
                return (int)this.GetValue(MinimumProperty);
            }
            set
            {
                this.SetValue(MinimumProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets the modulus.
        /// </summary>
        public int Modulus
        {
            get
            {
                return (int)this.GetValue(ModulusProperty);
            }
            set
            {
                this.SetValue(ModulusProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets the none caption.
        /// </summary>
        public string NoneCaption
        {
            get
            {
                return this.noneCaption;
            }

            set
            {
                this.noneCaption = value;
                this.RefreshNumberBox();
            }
        }

        /// <summary>
        /// Gets or sets the number.
        /// </summary>
        public int Number
        {
            get
            {
                return (int)this.GetValue(NumberProperty);
            }

            set
            {
                this.SetValue(NumberProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether select all on click.
        /// </summary>
        public bool SelectAllOnClick { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether show increment buttons.
        /// </summary>
        public bool ShowIncrementButtons { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether update binding on text change.
        /// </summary>
        public bool UpdateBindingOnTextChange { get; set; }

        /// <summary>
        /// Gets the increment.
        /// </summary>
        private int Increment
        {
            get
            {
                return this.Modulus > 0 ? this.Modulus : 1;
            }
        }

        #endregion

        #region Methods

        /// <summary>
        /// The on allow empty changed.
        /// </summary>
        /// <param name="dependencyObject">
        /// The dependency object.
        /// </param>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        private static void OnAllowEmptyChanged(
            DependencyObject dependencyObject, DependencyPropertyChangedEventArgs eventArgs)
        {
            var numBox = dependencyObject as TimeSpanBox;
            if (numBox != null)
            {
                numBox.RefreshNumberBox();
            }
        }

        /// <summary>
        /// The on number changed.
        /// </summary>
        /// <param name="dependencyObject">
        /// The dependency object.
        /// </param>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        private static void OnNumberChanged(
            DependencyObject dependencyObject, DependencyPropertyChangedEventArgs eventArgs)
        {
            if (eventArgs.NewValue != eventArgs.OldValue)
            {
                var numBox = dependencyObject as TimeSpanBox;

                if (!numBox.suppressRefresh)
                {
                    numBox.RefreshNumberBox();
                }
            }
        }

        /// <summary>
        /// The on show timespan number changed.
        /// </summary>
        /// <param name="dependencyObject">
        /// The dependency object.
        /// </param>
        /// <param name="eventArgs">
        /// The event args.
        /// </param>
        private static void OnShowTimeSpanChanged(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs eventArgs)
        {
            if (eventArgs.NewValue != eventArgs.OldValue)
            {
                var numBox = dependencyObject as TimeSpanBox;

                if (!numBox.suppressRefresh)
                {
                    numBox.RefreshNumberBox();
                }
            }
        }     

        /// <summary>
        /// The decrement number.
        /// </summary>
        private void DecrementNumber()
        {
            int newNumber;
            if (this.AllowEmpty && this.Number == 0)
            {
                newNumber = Math.Min(this.Maximum, -this.Increment);
            }
            else
            {
                newNumber = this.Number - this.Increment;
            }

            if (newNumber < this.Minimum)
            {
                newNumber = this.Minimum;
            }

            if (newNumber != this.Number)
            {
                this.Number = newNumber;
            }
        }

        /// <summary>
        /// The down button mouse left button down.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void DownButtonMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            this.refireControl = new RefireControl(this.DecrementNumber);
            this.refireControl.Begin();
        }

        /// <summary>
        /// The down button mouse left button up.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void DownButtonMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            this.refireControl.Stop();
        }

        /// <summary>
        /// The get nearest value.
        /// </summary>
        /// <param name="number">
        /// The number.
        /// </param>
        /// <param name="modulus">
        /// The modulus.
        /// </param>
        /// <returns>
        /// The <see cref="double"/>.
        /// </returns>
        private int GetNearestValue(int number, int modulus)
        {
            int remainder = number % modulus;

            if (remainder == 0)
            {
                return number;
            }

            return remainder >= (modulus / 2) ? number + (modulus - remainder) : number - remainder;
        }

        /// <summary>
        /// The increment number.
        /// </summary>
        private void IncrementNumber()
        {
            int newNumber;
            if (this.AllowEmpty && this.Number == 0)
            {
                newNumber = Math.Max(this.Minimum, this.Increment);
            }
            else
            {
                newNumber = this.Number + this.Increment;
            }

            if (newNumber > this.Maximum)
            {
                newNumber = this.Maximum;
            }

            if (newNumber != this.Number)
            {
                this.Number = newNumber;
            }
        }

        /// <summary>
        /// The number box got focus.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NumberBoxGotFocus(object sender, RoutedEventArgs e)
        {
            this.hasFocus = true;

            if (this.AllowEmpty)
            {
                if (this.Number == 0)
                {
                    this.numberBox.Text = string.Empty;
                }

                this.numberBox.Foreground = new SolidColorBrush(Colors.Black);
            }
        }

        /// <summary>
        /// The number box lost focus.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NumberBoxLostFocus(object sender, RoutedEventArgs e)
        {
            this.hasFocus = false;

            if (this.AllowEmpty && this.numberBox.Text == string.Empty)
            {
                this.Number = 0;
                this.RefreshNumberBox();
                return;
            }

            this.UpdateNumberBindingFromBox();
            this.RefreshNumberBox();
        }

        /// <summary>
        /// The number box preview key down.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NumberBoxPreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Space)
            {
                e.Handled = true;
            }
        }

        /// <summary>
        /// The number box preview mouse down.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NumberBoxPreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (!this.hasFocus)
            {
                this.lastFocusMouseDown = DateTime.Now;
            }
        }

        /// <summary>
        /// The number box preview mouse up.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NumberBoxPreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            // If this mouse up is soon enough after an initial click on the box, select all.
            if (this.SelectAllOnClick && DateTime.Now - this.lastFocusMouseDown < SelectAllThreshold)
            {
                this.Dispatcher.BeginInvoke(new Action(() => this.numberBox.SelectAll()));
            }
        }

        /// <summary>
        /// The number box preview text input.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NumberBoxPreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            if (e.Text.Any(c => !char.IsNumber(c) && c != '.' && c != ':' && (this.Minimum >= 0 || c != '-')))
            {
                e.Handled = true;
            }
        }

        /// <summary>
        /// The number box text changed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void NumberBoxTextChanged(object sender, TextChangedEventArgs e)
        {
            if (this.UpdateBindingOnTextChange)
            {
                if (this.AllowEmpty && this.numberBox.Text == string.Empty)
                {
                    this.Number = 0;
                    return;
                }

                this.UpdateNumberBindingFromBox();
            }

            this.RefreshNumberBoxColor();
        }

        /// <summary>
        /// The number is valid.
        /// </summary>
        /// <param name="number">
        /// The number.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        private bool NumberIsValid(double number)
        {
            return number >= this.Minimum && number <= this.Maximum;
        }

        /// <summary>
        /// The refresh number box.
        /// </summary>
        private void RefreshNumberBox()
        {
            if (this.AllowEmpty && this.Number == 0)
            {
                this.numberBox.Text = this.hasFocus ? string.Empty : this.NoneCaption;

                // this.numberBox.Foreground = new SolidColorBrush(Colors.Gray);
            }
            else
            {
                if (this.ShowTimeSpan)
                {
                    this.numberBox.Text = TimeSpan.FromSeconds(this.Number).ToString();
                }
                else
                {
                    this.numberBox.Text = this.Number.ToString(CultureInfo.InvariantCulture);
                }

                // this.numberBox.Foreground = new SolidColorBrush(Colors.Black);
            }

            this.RefreshNumberBoxColor();
        }

        /// <summary>
        /// The refresh number box color.
        /// </summary>
        private void RefreshNumberBoxColor()
        {
            this.numberBox.Foreground = this.numberBox.Text == this.NoneCaption ? new SolidColorBrush(Colors.Gray) : new SolidColorBrush(Colors.Black);
        }

        /// <summary>
        /// The up button mouse left button down.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void UpButtonMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            this.refireControl = new RefireControl(this.IncrementNumber);
            this.refireControl.Begin();
        }

        /// <summary>
        /// The up button mouse left button up.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void UpButtonMouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            this.refireControl.Stop();
        }

        /// <summary>
        /// The update number binding from box.
        /// </summary>
        private void UpdateNumberBindingFromBox()
        {
            int newNumber;
            TimeSpan newTimespanNumber;
            if (int.TryParse(this.numberBox.Text, out newNumber))
            {
                if (this.NumberIsValid(newNumber))
                {
                    if (this.Modulus != 0)
                    {
                        newNumber = this.GetNearestValue(newNumber, this.Modulus);
                    }

                    if (newNumber != this.Number)
                    {
                        // While updating the binding we don't need to react to the change.
                        this.suppressRefresh = true;
                        this.Number = newNumber;
                        this.suppressRefresh = false;
                    }
                }
            } 
            else if (TimeSpan.TryParse(this.numberBox.Text, out newTimespanNumber))
            {
                if (newTimespanNumber != TimeSpan.Zero)
                {
                    int seconds = (int)Math.Round(newTimespanNumber.TotalSeconds, 0);
                    if (seconds != this.Number)
                    {
                        // While updating the binding we don't need to react to the change.
                        this.suppressRefresh = true;
                        this.Number = seconds;
                        this.suppressRefresh = false;
                    }
                }
            }
        }

        /// <summary>
        /// The user control_ loaded.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (!this.ShowIncrementButtons)
            {
                this.incrementButtonsGrid.Visibility = Visibility.Collapsed;
            }
        }

        #endregion
    }
}