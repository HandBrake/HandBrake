/*  frmUpdater.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Net;

namespace Handbrake
{
    public partial class frmUpdater : Form
    {
        Functions.AppcastReader rssRead = new Functions.AppcastReader();
        public frmUpdater()
        {
            InitializeComponent();

            getRss();
            setVersions();
        }

        private void getRss()
        {
            wBrowser.DocumentText = "<font face=\"verdana\" size=\"1\">" + rssRead.versionInfo() + "</font>";
        }

        private void setVersions()
        {
            lbl_oldVersion.Text = "(you have: " + Properties.Settings.Default.hb_version + " / " + Properties.Settings.Default.hb_build + ").";
            lbl_newVersion.Text = rssRead.version() + " (" + rssRead.build() + ")";
        }

        private void btn_installUpdate_Click(object sender, EventArgs e)
        {
            frmDownload download = new frmDownload();
            download.Show();
            this.Close();
        }

        private void btn_remindLater_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void btn_skip_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.skipversion = int.Parse(rssRead.build());
            Properties.Settings.Default.Save();

            this.Close();
        }

    }
}