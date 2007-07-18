using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmAbout : Form
    {
        public frmAbout()
        {
            InitializeComponent();
            Version.Text = Properties.Settings.Default.GuiVersion;
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}