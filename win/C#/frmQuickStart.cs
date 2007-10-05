using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;

namespace Handbrake
{
    public partial class frmQuickStart : Form
    {
        public frmQuickStart()
        {
            InitializeComponent();
        }

        private void label_docs_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start("http://handbrake.m0k.org/trac/wiki/HandBrakeGuide");
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}