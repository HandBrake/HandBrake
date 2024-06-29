// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SimpleTimeControl.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// Interaction logic for SimpleTimeControl.xaml
    /// </summary>
    public partial class SimpleTimeControl : UserControl
    {
        public static readonly DependencyProperty HourProperty = DependencyProperty.Register(
            "Hour", typeof(int), typeof(SimpleTimeControl), new PropertyMetadata(0, OnHourChanged));
        
        public static readonly DependencyProperty MinuteProperty = DependencyProperty.Register(
            "Minute", typeof(int), typeof(SimpleTimeControl), new PropertyMetadata(0, OnMinuteChanged));

        public SimpleTimeControl()
        {
            InitializeComponent();
            this.DataContext = this;
            this.hour.SelectedItem = 0;
            this.minute.SelectedItem = 1;

            for (int i = 0; i <= 23; i++)
            {
                this.hour.Items.Add(i);
            }

            for (int i = 0; i <= 59; i++)
            {
                this.minute.Items.Add(i);
            }
        }

        public int Hour
        {
            get
            {
                return (int)this.GetValue(HourProperty);
            }
            set
            {
                this.SetValue(HourProperty, value);
            }
        }

        public int Minute
        {
            get
            {
                return (int)this.GetValue(MinuteProperty);
            }
            set
            {
                this.SetValue(MinuteProperty, value);
            }
        }

        private static void OnHourChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is SimpleTimeControl)
            {
                SimpleTimeControl ctlControl = (SimpleTimeControl)d;
                ctlControl.hour.SelectedItem = (int)e.NewValue;
            }
        }


        private static void OnMinuteChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is SimpleTimeControl)
            {
                SimpleTimeControl ctlControl = (SimpleTimeControl)d;
                ctlControl.minute.SelectedItem = (int)e.NewValue;
            }
        }


        private void Hour_OnSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.hour.SelectedItem == null || this.minute.SelectedItem == null)
            {
                return;
            }
            
            this.Hour = (int)this.hour.SelectedItem;
        }

        private void Minute_OnSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.hour.SelectedItem == null || this.minute.SelectedItem == null)
            {
                return;
            }

            this.Minute = (int)this.minute.SelectedItem;
        }
    }
}
