// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AboutView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AboutView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Views
{
    using System.Diagnostics;
    using System.Windows.Controls;
    using System.Windows.Navigation;

    /// <summary>
    /// Interaction logic for AboutView.xaml
    /// </summary>
    public partial class AboutView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AboutView"/> class.
        /// </summary>
        public AboutView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The handbrake website_ on request navigate.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void HandbrakeWebsite_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            Process.Start("https://handbrake.fr");
        }
    }
}
