using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmQuery : Form
    {

        public frmQuery(string query)
        {
            InitializeComponent();
            rtf_query.Text = query;
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void btn_copy_Click(object sender, EventArgs e)
        {
            if (rtf_query.Text != "")
                Clipboard.SetText(rtf_query.Text, TextDataFormat.Text);
        }

    }
}