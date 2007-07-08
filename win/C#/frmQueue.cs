using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmQueue : Form
    {
        public frmQueue()
        {
            InitializeComponent();
        }

        public void addItem(String item)
        {
            list_queue.Items.Add(item);
        }

        private void btn_Close_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}