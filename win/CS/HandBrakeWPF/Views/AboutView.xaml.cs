// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AboutView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AboutView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Diagnostics;
    using System.Windows.Controls;
    using System.Windows.Input;
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
            this.InitializeComponent();
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
            Process.Start("explorer.exe", "https://handbrake.fr");
        }

        private void GithubWebsite_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            Process.Start("explorer.exe", "https://github.com/HandBrake/HandBrake/issues");
        }

        private void DocWebsite_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            Process.Start("explorer.exe", "https://handbrake.fr/docs");
        }

        private void AboutTextBox_OnMouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.versionTextBox.SelectAll();
        }
    }
}
