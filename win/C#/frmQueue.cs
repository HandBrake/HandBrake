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
        private string query = "";

        public frmQueue(string query)
        {
            InitializeComponent();
            this.query = query;
        }

        public void addItem(String item)
        {
            list_queue.Items.Add(item);
            MessageBox.Show("test");
        }

        private void btn_Close_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void frmQueue_Load(object sender, EventArgs e)
        {
            addItem(query);
            MessageBox.Show("test");
        }


    }
}