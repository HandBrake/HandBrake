/*  frmUpdater.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using Handbrake.Functions;

namespace Handbrake
{
    public partial class frmUpdater : Form
    {
        AppcastReader appcast = new AppcastReader();
        public frmUpdater()
        {
            InitializeComponent();

            appcast.getInfo(); // Initializes the appcast
            getRss();
            setVersions();
        }

        private void getRss()
        {
            wBrowser.Url = appcast.descriptionUrl();
        }

        private void setVersions()
        {
            string old = "(You have: " + Properties.Settings.Default.hb_version.Trim() + " / " + Properties.Settings.Default.hb_build.ToString().Trim() + ")";
            string newBuild = appcast.version().Trim() + " (" + appcast.build() + ")";
            lbl_update_text.Text = "HandBrake " + newBuild + " is now available. " + old;
        }

        private void btn_installUpdate_Click(object sender, EventArgs e)
        {
            frmDownload download = new frmDownload(appcast.downloadFile());
            download.Show();
            this.Close();
        }

        private void btn_remindLater_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void btn_skip_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.skipversion = int.Parse(appcast.build());
            Properties.Settings.Default.Save();

            this.Close();
        }

    }
}