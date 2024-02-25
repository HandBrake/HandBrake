// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AddPresetView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AddPresetView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;

    using HandBrakeWPF.Helpers;
    using System.Windows;
    using HandBrakeWPF.Commands;
    using System.Windows.Input;

    /// <summary>
    /// Interaction logic for AddPresetView.xaml
    /// </summary>
    public partial class AddPresetView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AddPresetView"/> class.
        /// </summary>
        public AddPresetView()
        {
            InitializeComponent();
            this.InputBindings.Add(new InputBinding(new CloseWindowCommand(this), new KeyGesture(Key.W, ModifierKeys.Control))); // Close Window
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }
    }
}
