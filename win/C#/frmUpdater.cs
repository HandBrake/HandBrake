/*  frmUpdater.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Windows.Forms;
    using Functions;

    /// <summary>
    /// A window to display update information.
    /// </summary>
    public partial class frmUpdater : Form
    {
        /// <summary>
        /// An instance of the Appcast Reader
        /// </summary>
        private readonly AppcastReader appcast;

        /// <summary>
        /// Initializes a new instance of the <see cref="frmUpdater"/> class.
        /// </summary>
        /// <param name="reader">
        /// The appcast reader.
        /// </param>
        public frmUpdater(AppcastReader reader)
        {
            InitializeComponent();

            appcast = reader;
            GetRss();
            SetVersions();
        }

        /// <summary>
        /// Get the RSS feed
        /// </summary>
        private void GetRss()
        {
            wBrowser.Url = appcast.DescriptionUrl;
        }

        /// <summary>
        /// Set the versions
        /// </summary>
        private void SetVersions()
        {
            string old = "(You have: " + Properties.Settings.Default.hb_version.Trim() + " / " +
                         Properties.Settings.Default.hb_build.ToString().Trim() + ")";
            string newBuild = appcast.Version.Trim() + " (" + appcast.Build + ")";
            lbl_update_text.Text = "HandBrake " + newBuild + " is now available. " + old;
        }

        /// <summary>
        /// Handle the Install Update button click event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void BtnInstallUpdateClick(object sender, EventArgs e)
        {
            frmDownload download = new frmDownload(appcast.DownloadFile);
            download.ShowDialog();
            this.Close();
        }

        /// <summary>
        /// Handle the Remind Later button click event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void BtnRemindLaterClick(object sender, EventArgs e)
        {
            this.Close();
        }

        /// <summary>
        /// Handle the Skip update button click event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnSkipClick(object sender, EventArgs e)
        {
            Properties.Settings.Default.skipversion = int.Parse(appcast.Build);
            Properties.Settings.Default.Save();

            this.Close();
        }
    }
}