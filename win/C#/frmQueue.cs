using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Handbrake
{
    public partial class frmQueue : Form
    {
        // Declarations
        private delegate void ProgressUpdateHandler();
        private delegate void getQueueItem();
        private delegate void setEncoding();
        private delegate void modifyQueue();

        // Everything starts Here
        public frmQueue()
        {
            InitializeComponent();
        }

        #region encode queue Handlnig

        // Declarations
        Boolean cancel = false;
        string foundQuery = "";
        [DllImport("user32.dll")]
        public static extern void LockWorkStation();
        [DllImport("user32.dll")]
        public static extern int ExitWindowsEx(int uFlags, int dwReason);

        // Methods
        private void btn_q_encoder_Click(object sender, EventArgs e)
        {
            // Reset some values

            lbl_status.Visible = false;
            cancel = false;

            // Start the encode
            try
            {
                if (listview_queue.Items.Count != 0)
                {
                    // Setup or reset some values
                    btn_cancel.Visible = true;
                    progressBar.Value = 0;
                    lbl_progressValue.Text = "0 %";
                    progressBar.Step = 100 / listview_queue.Items.Count;
                    progressBar.Update();
                    //ThreadPool.QueueUserWorkItem(startProc);
                    // Testing a new way of launching a thread. Hopefully will fix a random freeze up of the main thread.
                    Thread theQ = new Thread(startProc);
                    theQ.Start();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        private void btn_cancel_Click(object sender, EventArgs e)
        {
            cancel = true;
            btn_cancel.Visible = false;
            MessageBox.Show("No further items on the queue will start. The current encode process will continue until it is finished. \nClick 'Encode Video' when you wish to continue encoding the queue.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        private void startProc(object state)
        {
            try
            {
                while (listview_queue.Items.Count != 0)
                {
                    getItem();
                    string query = foundQuery;
                    query = query.Replace("ListViewItem: { ", "").Replace(" }", "").Trim();
                    updateUIElements();
                    setEncValue();

                    Functions.CLI process = new Functions.CLI();
                    Process hbProc = process.runCli(this, query, false, false, false, false);

                    hbProc.WaitForExit();
                    hbProc.Close();
                    hbProc.Dispose();

                    query = "";

                    if (cancel == true)
                    {
                        break;
                    }

                }

                resetQueue();

                // Do something whent he encode ends.
                switch (Properties.Settings.Default.CompletionOption)
                {
                    case "Shutdown":
                        System.Diagnostics.Process.Start("Shutdown", "-s -t 60");
                        break;
                    case "Log Off":
                        ExitWindowsEx(0, 0);
                        break;
                    case "Suspend":
                        Application.SetSuspendState(PowerState.Suspend, true, true);
                        break;
                    case "Hibernate":
                        Application.SetSuspendState(PowerState.Hibernate, true, true);
                        break;
                    case "Lock System":
                        LockWorkStation();
                        break;
                    case "Quit HandBrake":
                        Application.Exit();
                        break;
                    default:
                        break;
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        private void getItem()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new getQueueItem(getItem));
                }
                foundQuery = this.listview_queue.Items[0].ToString();
            }
            catch (Exception)
            {
                // Do Nothing
            }
        }

        private void setEncValue()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new setEncoding(setEncValue));
                }

                string query = null;
                ListView.SelectedListViewItemCollection name = null;
                name = listview_queue.SelectedItems;

                if (listview_queue.SelectedItems.Count != 0)
                    query = name[0].SubItems[0].Text;

                // found query is a global varible
                Functions.QueryParser parsed = Functions.QueryParser.Parse(foundQuery);
                lbl_source.Text = parsed.Source;
                lbl_dest.Text = parsed.Destination;


                if (parsed.DVDTitle == 0)
                    lbl_title.Text = "Auto";
                else
                    lbl_title.Text = parsed.DVDTitle.ToString();

                string chatpers = "";
                if (parsed.DVDChapterStart == 0)
                {
                    lbl_chapt.Text = "Auto";
                }
                else
                {
                    chatpers = parsed.DVDChapterStart.ToString();
                    if (parsed.DVDChapterFinish != 0)
                        chatpers = chatpers + " - " + parsed.DVDChapterFinish;
                    lbl_chapt.Text = parsed.DVDChapterStart + chatpers;
                }

                lbl_vEnc.Text = parsed.VideoEncoder;
                lbl_aEnc.Text = parsed.AudioEncoder;
            }
            catch (Exception)
            {
                // Do Nothing
            }

        }

        private void updateUIElements()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new ProgressUpdateHandler(updateUIElements));
                    return;
                }
                this.listview_queue.Items.RemoveAt(0);

                progressBar.PerformStep();
                lbl_progressValue.Text = string.Format("{0} %", progressBar.Value);
                progressBar.Update();
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        private void resetQueue()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new ProgressUpdateHandler(resetQueue));
                    return;
                }

                if (cancel == true)
                {
                    lbl_status.Visible = true;
                    lbl_status.Text = "Encode Queue Cancelled!";
                }
                else
                {
                    lbl_status.Visible = true;
                    lbl_status.Text = "Encode Queue Completed!";
                }
                btn_cancel.Visible = false;

                lbl_progressValue.Text = "0 %";
                progressBar.Value = 0;
                progressBar.Update();

                lbl_source.Text = "-";
                lbl_dest.Text = "-";
                lbl_vEnc.Text = "-";
                lbl_aEnc.Text = "-";
                lbl_title.Text = "-";
                lbl_chapt.Text = "-";
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        #endregion

        #region Queue Management
        private void btn_up_Click(object sender, EventArgs e)
        {
            string cache;
            int selIdx;
            if (listview_queue.Items.Count != 0)
            {
                selIdx = listview_queue.SelectedItems[0].Index;
                // ignore moveup of row(0)
                if (selIdx == 0)
                    return;

                // move the subitems for the previous row
                // to cache to make room for the selected row
                for (int i = 0; i < listview_queue.Items[selIdx].SubItems.Count; i++)
                {
                    cache = listview_queue.Items[selIdx - 1].SubItems[i].Text;
                    listview_queue.Items[selIdx - 1].SubItems[i].Text =
                      listview_queue.Items[selIdx].SubItems[i].Text;
                    listview_queue.Items[selIdx].SubItems[i].Text = cache;
                }
                listview_queue.Items[selIdx - 1].Selected = true;
                listview_queue.Refresh();
                listview_queue.Focus();
            }
        }

        private void btn_down_Click(object sender, EventArgs e)
        {
            string cache;
            int selIdx;
            
            if (listview_queue.Items.Count != 0)
            {
                selIdx = listview_queue.SelectedItems[0].Index;

                // ignore movedown of last item
                if (selIdx == listview_queue.Items.Count - 1)
                    return;
                // move the subitems for the next row
                // to cache so we can move the selected row down
                for (int i = 0; i < listview_queue.Items[selIdx].SubItems.Count; i++)
                {
                    cache = listview_queue.Items[selIdx + 1].SubItems[i].Text;
                    listview_queue.Items[selIdx + 1].SubItems[i].Text =
                      listview_queue.Items[selIdx].SubItems[i].Text;
                    listview_queue.Items[selIdx].SubItems[i].Text = cache;
                }
                listview_queue.Items[selIdx + 1].Selected = true;
                listview_queue.Refresh();
            listview_queue.Focus();
            }
        }

        private void btn_delete_Click(object sender, EventArgs e)
        {
            for (int i = listview_queue.SelectedItems.Count - 1; i >= 0; i--)
            {
                ListViewItem item = listview_queue.SelectedItems[i];
                listview_queue.Items.Remove(item);
            }
        }
        #endregion

        #region Modify Queue

        int listCount = 0;
        private void listview_queue_SelectedIndexChanged(object sender, EventArgs e)
        {
            modQ();
            listCount = listview_queue.Items.Count;
        }

        private void modQ()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new modifyQueue(modQ));
                }

                string query = null;
                ListView.SelectedListViewItemCollection name = null;
                name = listview_queue.SelectedItems;

                if (listview_queue.SelectedItems.Count != 0)
                    query = name[0].SubItems[0].Text;

                txt_editQuery.Text = query;

            }
            catch (Exception)
            {
            }
        }

        private void btn_update_Click(object sender, EventArgs e)
        {
            try
            {
                if (txt_editQuery.Text != "")
                {
                    if (listview_queue.Items.Count != listCount)
                    {
                        MessageBox.Show("Unable to modify the selected item. The number of items on the list has changed.  \nPlease avoid modifying an item when a new encode is about to start!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                    else
                    {
                        if (listview_queue.SelectedItems != null)
                        {
                            int selectItm = listview_queue.SelectedIndices[0];
                            listview_queue.Items[selectItm].Text = txt_editQuery.Text;
                        }
                    }
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        #endregion

        protected override void OnClosing(CancelEventArgs e)
        {
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }

        private void btn_Close_Click(object sender, EventArgs e)
        {
            this.Hide();
        }

    }
}