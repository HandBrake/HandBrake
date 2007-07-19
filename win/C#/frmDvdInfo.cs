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
        /// <summary>
        /// This window should be used to display the RAW output of the handbrake CLI which is produced during the scan.
        /// </summary>
        public frmDvdInfo()
        {
            InitializeComponent();
            this.rtf_dvdInfo.Text = string.Empty;
        }

        public void HandleParsedData(object Sender, string Data)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Parsing.DataReadEventHandler(HandleParsedData), new object[] { Sender, Data });
                return;
            }
            this.rtf_dvdInfo.AppendText(Data + System.Environment.NewLine);
        }

        private void btn_close_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }
    }
}