using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmDVDData : Form
    {

        /*
         * This window will be used to display the raw output of hbcli.exe when it is encoding.
         * 
         */

        public frmDVDData()
        {
            InitializeComponent();
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void frmDVDData_Load(object sender, EventArgs e)
        {

        }
    }
}