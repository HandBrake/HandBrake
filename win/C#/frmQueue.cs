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
    public partial class frmQueue : Form
    {
        private delegate void ProgressUpdateHandler();

        public frmQueue()
        {
            InitializeComponent();
        }

        private void btn_Close_Click(object sender, EventArgs e)
        {
            this.Hide();
        }

        private void btn_delete_Click(object sender, EventArgs e)
        {
            list_queue.Items.Remove(list_queue.SelectedItem);
        }

        private void btn_up_Click(object sender, EventArgs e)
        {
            int count = list_queue.Items.Count;
            int itemToMove = list_queue.SelectedIndex;
            int previousItemint = 0; 
            String previousItem = "";

             if (itemToMove > 0){
                previousItemint = itemToMove - 1;
                previousItem = list_queue.Items[previousItemint].ToString();
                list_queue.Items[previousItemint] = list_queue.Items[itemToMove];
                list_queue.Items[itemToMove] = previousItem;
                list_queue.SelectedIndex = list_queue.SelectedIndex - 1;
            }
        }

        private void btn_down_Click(object sender, EventArgs e)
        {
            int count = list_queue.Items.Count;
            int itemToMove = list_queue.SelectedIndex;
            int itemAfterInt = 0; 
            String itemAfter = "";

            if (itemToMove < (count - 1))
            {
                itemAfterInt = itemToMove + 1;
                itemAfter = list_queue.Items[itemAfterInt].ToString();
                list_queue.Items[itemAfterInt] = list_queue.Items[itemToMove];
                list_queue.Items[itemToMove] = itemAfter;
                list_queue.SelectedIndex = list_queue.SelectedIndex + 1;
            }
        }

        private void btn_q_encoder_Click(object sender, EventArgs e)
        {
            progressBar.Value = 0;
            lbl_progressValue.Text = "0 %";
            progressBar.Step = 100 / list_queue.Items.Count;
            progressBar.Update();
            ThreadPool.QueueUserWorkItem(startProc);
        }

        private void startProc(object state)
        {
            int initialListCount = list_queue.Items.Count;
            for (int i = 0; i < initialListCount; i++)
            {
                string query = list_queue.Items[0].ToString();

                Functions.CLI process = new Functions.CLI();
                Process hbProc = process.runCli(this, query, false, false, false, false);
       
                hbProc.WaitForExit();
                hbProc.Close();
                hbProc.Dispose();

                updateUIElements();
            }
        }

        private void updateUIElements()
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new ProgressUpdateHandler(updateUIElements));
                return;
            }
            this.list_queue.Items.RemoveAt(0);
            progressBar.PerformStep();
            lbl_progressValue.Text = string.Format("{0} %", progressBar.Value);
            progressBar.Update();

            if (progressBar.Value == 100)
            {
                MessageBox.Show("Encode Queue Completed!", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                progressBar.Value = 0;
                progressBar.Update();
                lbl_progressValue.Text = "0 %";
            }
        }
    }
}