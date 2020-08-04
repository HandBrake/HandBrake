// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NumberBox.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for NumberBox.xaml
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
    ///     Interaction logic for NumberBox.xaml
    /// </summary>
    public partial class NumberBox
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
            "AllowEmpty", typeof(bool), typeof(NumberBox), new PropertyMetadata(true, OnAllowEmptyChanged));

        /// <summary>
        /// The maximum property.
        /// </summary>
        public static readonly DependencyProperty MaximumProperty = DependencyProperty.Register(
            MaximumPropertyName, typeof(double?), typeof(NumberBox), new UIPropertyMetadata(double.MaxValue));

        /// <summary>
        /// The minimum property.
        /// </summary>
        public static readonly DependencyProperty MinimumProperty = DependencyProperty.Register(
            MinimumPropertyName, typeof(double?), typeof(NumberBox), new UIPropertyMetadata(double.MinValue));

        /// <summary>
        /// The modulus property.
        /// </summary>
        public static readonly DependencyProperty ModulusProperty = DependencyProperty.Register(
            ModulusPropertyName, typeof(double), typeof(NumberBox), new UIPropertyMetadata(0.0));

        /// <summary>
        /// The number property.
        /// </summary>
        public static readonly DependencyProperty NumberProperty = DependencyProperty.Register(
            "Number", typeof(double?), typeof(NumberBox), new PropertyMetadata(OnNumberChanged));

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
        /// Initializes a new instance of the <see cref="NumberBox"/> class.
        /// </summary>
        public NumberBox()
        {
            this.noneCaption = string.Empty;
            this.UpdateBindingOnTextChange = true;
            this.ShowIncrementButtons = true;
            this.SelectAllOnClick = true;

            this.InitializeComponent();

            this.RefreshNumberBox();
        }

        #endregion

        #region Properties

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
        public double? Maximum
        {
            get
            {
                return (double?)this.GetValue(MaximumProperty);
            }
            set
            {
                this.SetValue(MaximumProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets the minimum.
        /// </summary>
        public double? Minimum
        {
            get
            {
                return (double?)this.GetValue(MinimumProperty);
            }
            set
            {
                this.SetValue(MinimumProperty, value);
            }
        }

        /// <summary>
        /// Gets or sets the modulus.
        /// </summary>
        public double Modulus
        {
            get
            {
                return (double)this.GetValue(ModulusProperty);
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
        public double? Number
        {
            get
            {
                return (double?)this.GetValue(NumberProperty);
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
        private double Increment
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
            var numBox = dependencyObject as NumberBox;
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
                var numBox = dependencyObject as NumberBox;

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
            if (!this.Number.HasValue)
            {
                this.Number = 0; // Default to 0
            }

            double newNumber;
            if (this.AllowEmpty && this.Number == 0)
            {
                if (this.Maximum.HasValue)
                {
                    newNumber = Math.Min(this.Maximum.Value, -this.Increment);
                }
                else
                {
                    newNumber = -this.Increment;
                }
            }
            else
            {
                newNumber = this.Number.Value - this.Increment;
            }

            if (this.Minimum.HasValue && newNumber < this.Minimum)
            {
                newNumber = this.Minimum.Value;
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
            this.refireControl?.Stop();
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
        private double GetNearestValue(double number, double modulus)
        {
            double remainder = number % modulus;

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
            if (!this.Number.HasValue)
            {
                this.Number = 0; // Default to 0
            }

            double newNumber;
            if (this.AllowEmpty && this.Number == 0)
            {
                if (this.Maximum.HasValue)
                {
                    newNumber = Math.Max(this.Minimum.Value, this.Increment);
                }
                else
                {
                    newNumber = this.Increment;
                }
            }
            else
            {
                newNumber = this.Number.Value + this.Increment;
            }

            if (this.Maximum.HasValue && newNumber > this.Maximum)
            {
                newNumber = this.Maximum.Value;
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

                this.numberBox.ClearValue(TextBox.ForegroundProperty);
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

            if (e.Key == Key.Up)
            {
                this.IncrementNumber();
            }
            else if (e.Key == Key.Down)
            {
                this.DecrementNumber();
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
            if (e.Text.Any(c => !char.IsNumber(c) && c != '.' && (this.Minimum >= 0 || c != '-')))
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
            if (!this.Number.HasValue)
            {
                return;
            }

            if (this.AllowEmpty && this.Number == 0)
            {
                this.numberBox.Text = this.hasFocus ? string.Empty : this.NoneCaption;
            }
            else
            {
                this.numberBox.Text = this.Number.Value.ToString(CultureInfo.InvariantCulture);
            }

            this.RefreshNumberBoxColor();
        }

        /// <summary>
        /// The refresh number box color.
        /// </summary>
        private void RefreshNumberBoxColor()
        {
            if (this.numberBox.Text == this.NoneCaption)
            {
                this.numberBox.Foreground = new SolidColorBrush(Colors.Gray);
            }
            else 
            {
                this.numberBox.ClearValue(TextBox.ForegroundProperty);
            }
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
            this.refireControl?.Stop();
        }

        /// <summary>
        /// The update number binding from box.
        /// </summary>
        private void UpdateNumberBindingFromBox()
        {
            double newNumber;
            if (double.TryParse(this.numberBox.Text, out newNumber))
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