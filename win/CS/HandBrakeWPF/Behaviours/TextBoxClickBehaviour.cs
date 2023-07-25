// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TextBoxBehavior.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------


namespace HandBrakeWPF.Behaviours
{
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;

    public static class TextBoxBehavior
    {
        public static readonly DependencyProperty TripleClickSelectAllProperty = 
            DependencyProperty.RegisterAttached("EnableTrippleClick", typeof(bool), typeof(TextBoxBehavior), new PropertyMetadata(false, OnPropertyChanged));

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var textbox = d as TextBox;
            if (textbox != null)
            {
                var enable = (bool)e.NewValue;
                if (enable)
                {
                    textbox.PreviewMouseLeftButtonDown += OnTextBoxMouseDown;
                }
                else
                {
                    textbox.PreviewMouseLeftButtonDown -= OnTextBoxMouseDown;
                }
            }
        }

        private static void OnTextBoxMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 3 && sender is TextBox box)
            {
                box.SelectAll();
            }
        }

        public static void SetTripleClickSelectAll(DependencyObject element, bool value)
        {
            element.SetValue(TripleClickSelectAllProperty, value);
        }

        public static bool GetTripleClickSelectAll(DependencyObject element)
        {
            return (bool)element.GetValue(TripleClickSelectAllProperty);
        }
    }
}
