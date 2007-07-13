using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmDvdInfo : Form
    {

        /*
         * This window should be used to display the RAW output of the handbrake CLI which is produced during the scan.
         */

        public frmDvdInfo()
        {
            InitializeComponent();
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}