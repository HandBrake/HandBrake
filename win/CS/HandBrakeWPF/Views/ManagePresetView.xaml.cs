// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ManagePresetView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for ManagePresetView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System;

    using HandBrakeWPF.Helpers;
    using System.Windows;

    /// <summary>
    /// Interaction logic for ManagePresetView.xaml
    /// </summary>
    public partial class ManagePresetView : Window
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ManagePresetView"/> class.
        /// </summary>
        public ManagePresetView()
        {
            InitializeComponent();
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }
    }
}
