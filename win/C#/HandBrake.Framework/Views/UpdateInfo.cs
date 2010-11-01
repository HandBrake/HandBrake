/*  UpdateInfo.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework.Views
{
    using System;
    using System.Windows.Forms;

    using HandBrake.Framework.Services.Interfaces;

    /// <summary>
    /// A window to display update information.
    /// </summary>
    public partial class UpdateInfo : Form
    {
        /// <summary>
        /// An instance of the Appcast Reader
        /// </summary>
        private readonly IAppcastReader appcast;

        /// <summary>
        /// The Current Version
        /// </summary>
        private readonly string currentVersion;

        /// <summary>
        /// The Current Build
        /// </summary>
        private readonly string currentBuild;

        /// <summary>
        /// Initializes a new instance of the <see cref="UpdateInfo"/> class.
        /// </summary>
        /// <param name="reader">
        /// The appcast reader.
        /// </param>
        /// <param name="currentVersion">
        /// The current Version.
        /// </param>
        /// <param name="currentBuild">
        /// The current Build.
        /// </param>
        public UpdateInfo(IAppcastReader reader, string currentVersion, string currentBuild)
        {
            InitializeComponent();

            appcast = reader;
            this.currentVersion = currentVersion;
            this.currentBuild = currentBuild;
            GetRss();
            SetVersions();
        }
        
        /// <summary>
        /// Gets the SkipVersion number
        /// </summary>
        public int SkipVersion { get; private set; }


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
            string old = string.Format("(You have: {0} / {1})", this.currentVersion, this.currentBuild);
            string newBuild = appcast.Version.Trim() + " (" + appcast.Build + ")";
            lbl_update_text.Text = string.Format("HandBrake {0} is now available. {1}", newBuild, old);
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
            DownloadUpdate download = new DownloadUpdate(appcast.DownloadFile);
            download.ShowDialog();
            this.DialogResult = DialogResult.OK;
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
            this.DialogResult = DialogResult.Cancel;
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
            this.SkipVersion = int.Parse(appcast.Build);
            this.DialogResult = DialogResult.OK;
        }
    }
}