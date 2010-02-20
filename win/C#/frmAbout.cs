/*  frmAbout.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Windows.Forms;

    public partial class frmAbout : Form
    {
        public frmAbout()
        {
            InitializeComponent();
            lbl_HBBuild.Text = Properties.Settings.Default.hb_version + " (" + Properties.Settings.Default.hb_build +
                               ") - " + Properties.Settings.Default.hb_platform;
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}