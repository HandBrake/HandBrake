/*  frmAbout.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.m0k.org/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;

namespace Handbrake
{
    public partial class frmAbout : Form
    {
        public frmAbout()
        {
            InitializeComponent();
            Version.Text = Properties.Settings.Default.hb_version;
            lbl_build.Text = Properties.Settings.Default.hb_build.ToString();
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void label_credits_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start("http://handbrake.m0k.org/trac/wiki/x264Options");
        }
    }
}