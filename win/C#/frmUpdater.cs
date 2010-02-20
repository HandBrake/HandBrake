/*  frmUpdater.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Windows.Forms;
    using Functions;

    public partial class frmUpdater : Form
    {
        private readonly AppcastReader Appcast;

        public frmUpdater(AppcastReader reader)
        {
            InitializeComponent();

            Appcast = reader;
            GetRss();
            SetVersions();
        }

        private void GetRss()
        {
            wBrowser.Url = Appcast.DescriptionUrl;
        }

        private void SetVersions()
        {
            string old = "(You have: " + Properties.Settings.Default.hb_version.Trim() + " / " +
                         Properties.Settings.Default.hb_build.ToString().Trim() + ")";
            string newBuild = Appcast.Version.Trim() + " (" + Appcast.Build + ")";
            lbl_update_text.Text = "HandBrake " + newBuild + " is now available. " + old;
        }

        private void btn_installUpdate_Click(object sender, EventArgs e)
        {
            frmDownload download = new frmDownload(Appcast.DownloadFile);
            download.ShowDialog();
            this.Close();
        }

        private void btn_remindLater_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void btn_skip_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.skipversion = int.Parse(Appcast.Build);
            Properties.Settings.Default.Save();

            this.Close();
        }
    }
}