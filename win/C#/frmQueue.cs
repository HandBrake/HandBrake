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
using System.IO;

namespace Handbrake
{
    public partial class frmQueue : Form
    {
        private delegate void ProgressUpdateHandler();
        private delegate void setEncoding();

        public frmQueue()
        {
            InitializeComponent();
        }

        #region Queue Handling
        Boolean cancel = false;

        private void btn_encode_Click(object sender, EventArgs e)
        {
            // Reset some values

            if (list_queue.Items.Count != 0)
            {
                lbl_status.Visible = false;
                btn_encode.Enabled = false;
            }
            cancel = false;

            // Start the encode
            try
            {
                if (list_queue.Items.Count != 0)
                {
                    // Setup or reset some values
                    btn_stop.Visible = true;
                    progressBar.Value = 0;
                    lbl_progressValue.Text = "0 %";
                    progressBar.Step = 100 / list_queue.Items.Count;
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

        private void btn_stop_Click(object sender, EventArgs e)
        {
            cancel = true;
            btn_stop.Visible = false;
            btn_encode.Enabled = true;
            MessageBox.Show("No further items on the queue will start. The current encode process will continue until it is finished. \nClick 'Encode Video' when you wish to continue encoding the queue.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        [DllImport("user32.dll")]
        public static extern void LockWorkStation();
        [DllImport("user32.dll")]
        public static extern int ExitWindowsEx(int uFlags, int dwReason);

        private void startProc(object state)
        {
            try
            {
                while (list_queue.Items.Count != 0)
                {
                    string query = list_queue.Items[0].ToString();
                    setEncValue();
                    updateUIElements();
                   
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

        private void updateUIElements()
        {
            try
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
                btn_stop.Visible = false;
                btn_encode.Enabled = true;

                if (cancel == true)
                {
                    lbl_status.Visible = true;
                    lbl_status.Text = "Encode Queue Cancelled!";
                    text_edit.Text = "";
                }
                else
                {
                    lbl_status.Visible = true;
                    lbl_status.Text = "Encode Queue Completed!";
                    text_edit.Text = "";
                }
                
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
        
        private void setEncValue()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new setEncoding(setEncValue));
                }

                string query = query = list_queue.Items[0].ToString();

                // found query is a global varible
                Functions.QueryParser parsed = Functions.QueryParser.Parse(query);
                lbl_source.Text = parsed.Source;
                lbl_dest.Text = parsed.Destination;


                if (parsed.DVDTitle == 0)
                    lbl_title.Text = "Auto";
                else
                    lbl_title.Text = parsed.DVDTitle.ToString();

                string chapters = "";
                if (parsed.DVDChapterStart == 0)
                {
                    lbl_chapt.Text = "Auto";
                }
                else
                {
                    chapters = parsed.DVDChapterStart.ToString();
                    if (parsed.DVDChapterFinish != 0)
                        chapters = chapters + " - " + parsed.DVDChapterFinish;
                    lbl_chapt.Text = chapters;
                }

                lbl_vEnc.Text = parsed.VideoEncoder;
                lbl_aEnc.Text = parsed.AudioEncoder;
            }
            catch (Exception)
            {
                // Do Nothing
            }
        }

        #endregion

        #region Queue Management

        private void btn_up_Click(object sender, EventArgs e)
        {
            int count = list_queue.Items.Count;
            int itemToMove = list_queue.SelectedIndex;
            int previousItemint = 0;
            String previousItem = "";

            if (itemToMove > 0)
            {
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

        private void btn_delete_Click(object sender, EventArgs e)
        {
            list_queue.Items.Remove(list_queue.SelectedItem);
        }

        #endregion

        #region Queue Item Modification

        int listCount = 0;

        private void btn_updateQuery_Click(object sender, EventArgs e)
        {
            if (text_edit.Text != "")
            {
                if (list_queue.Items.Count != listCount)
                {
                    MessageBox.Show("Unable to modify the selected item. The number of items on the list has changed.  \nPlease avoid modifying an item when a new encode is about to start!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else
                {
                    if (list_queue.SelectedItem != null)
                        list_queue.Items[list_queue.SelectedIndex] = text_edit.Text;
          
                }
            }
        }

        private void list_queue_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (list_queue.SelectedItem != null)
                text_edit.Text = list_queue.SelectedItem.ToString();

            listCount = list_queue.Items.Count;
        }

        #endregion

        #region Queue Save & Batch Script
        private void btn_batch_Click(object sender, EventArgs e)
        {
            string queries = "";
            for (int i = 0; i < list_queue.Items.Count; i++)
            {
                string query = list_queue.Items[i].ToString();
                string fullQuery = '"' + Application.StartupPath.ToString()+ "\\HandBrakeCLI.exe"+ '"' + query;

                if (queries == "")
                    queries = queries + fullQuery;
                else
                    queries = queries + " && " + fullQuery;
            }
            string strCmdLine = queries;

            SaveFile.ShowDialog();
            string filename = SaveFile.FileName;

            if (filename != "")
            {
                try
                {
                    // Create a StreamWriter and open the file
                    StreamWriter line = new StreamWriter(filename);

                    // Write the batch file query to the file
                    line.WriteLine(strCmdLine);

                    // close the stream
                    line.Close();

                    MessageBox.Show("Your batch script has been sucessfully saved.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk);
                }
                catch (Exception)
                {
                    MessageBox.Show("Unable to write to the file. Please make sure that the location has the correct permissions for file writing.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                }

            }
        }

        #endregion

        #region other
        private void btn_Close_Click(object sender, EventArgs e)
        {
            this.Hide();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }
        #endregion
    }
}