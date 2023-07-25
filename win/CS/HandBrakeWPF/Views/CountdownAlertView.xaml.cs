// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CountdownAlertView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for CountdownAlertView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;

    using HandBrakeWPF.Helpers;
    using System.Windows;

    /// <summary>
    /// Interaction logic for CountdownAlertView.xaml
    /// </summary>
    public partial class CountdownAlertView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CountdownAlertView"/> class. 
        /// </summary>
        public CountdownAlertView()
        {
            this.InitializeComponent();
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }
    }
}
