using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Net;

namespace Handbrake
{
    public partial class frmUpdater : Form
    {
        Functions.RssReader rssRead = new Functions.RssReader();
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
            lbl_oldVersion.Text = "(you have: " + Properties.Settings.Default.CliVersion + " / " + Properties.Settings.Default.build  + ").";
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
            try
            {
                Properties.Settings.Default.skipversion = int.Parse(rssRead.build());
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
            this.Close();
        }

    }
}