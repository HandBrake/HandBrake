/*  frmQueue.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.Collections;
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
        Functions.Encode cliObj = new Functions.Encode();
        Boolean cancel = false;
        Process hbProc = null;
        Functions.Queue queue;
        frmMain mainWindow = null;

        public frmQueue(frmMain main)
        {
            InitializeComponent();
            mainWindow = main;
        }

        /// <summary>
        /// Initializes the Queue list with the Arraylist from the Queue class
        /// </summary>
        /// <param name="qw"></param>
        public void setQueue(Functions.Queue qw)
        {
            queue = qw;
            redrawQueue();
            lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
        }

        /// <summary>
        /// Returns if there is currently an item being encoded by the queue
        /// </summary>
        /// <returns>Boolean true if encoding</returns>
        public Boolean isEncoding()
        {
            if (hbProc == null)
                return false;
            else
                return true;
        }

        // Redraw's the queue with the latest data from the Queue class
        private void redrawQueue()
        {
            list_queue.Items.Clear();
            ArrayList theQueue = queue.getQueue();
            foreach (ArrayList queue_item in theQueue)
            {
                string q_item = queue_item[1].ToString();
                Functions.QueryParser parsed = Functions.QueryParser.Parse(q_item);

                // Get the DVD Title
                string title = "";
                if (parsed.DVDTitle == 0)
                    title = "Auto";
                else
                    title = parsed.DVDTitle.ToString();

                // Get the DVD Chapters
                string chapters = "";
                if (parsed.DVDChapterStart == 0)
                    chapters = "Auto";
                else
                {
                    chapters = parsed.DVDChapterStart.ToString();
                    if (parsed.DVDChapterFinish != 0)
                        chapters = chapters + " - " + parsed.DVDChapterFinish;
                }

                ListViewItem item = new ListViewItem();
                item.Text = title; // Title
                item.SubItems.Add(chapters); // Chapters
                item.SubItems.Add(parsed.Source); // Source
                item.SubItems.Add(parsed.Destination); // Destination
                item.SubItems.Add(parsed.VideoEncoder); // Video
                item.SubItems.Add(parsed.AudioEncoder1); // Audio

                list_queue.Items.Add(item);
            }
        }

        // Initializes the encode process
        private void btn_encode_Click(object sender, EventArgs e)
        {
            mainWindow.setLastAction("encode");
            
            if (queue.count() != 0)
            {
                btn_encode.Enabled = false;
            }
            cancel = false;

            // Start the encode
            try
            {
                if (queue.count() != 0)
                {
                    // Setup or reset some values
                    btn_stop.Visible = true;
                    progressBar.Value = 0;
                    lbl_progressValue.Text = "0 %";
                    progressBar.Step = 100 / queue.count();
                    Thread theQ = new Thread(startProc);
                    theQ.IsBackground = true;
                    theQ.Start();
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        // Starts the encoding process
        private void startProc(object state)
        {
            try
            {
                // Run through each item on the queue
                while (queue.count() != 0)
                {
                    string query = queue.getNextItemForEncoding();
                    queue.write2disk("hb_queue_recovery.dat"); // Update the queue recovery file

                    setEncValue();
                    updateUIElements();

                    hbProc = cliObj.runCli(this, query);

                    hbProc.WaitForExit();
                    hbProc.Close();
                    hbProc.Dispose();
                    hbProc = null;

                    query = "";

                    if (cancel == true)
                    {
                        break;
                    }
                }

                resetQueue();

                // After the encode is done, we may want to shutdown, suspend etc.
                cliObj.afterEncodeAction();
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        // Reset's the window to the default state.
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
                    lbl_progressValue.Text = "Encode Queue Cancelled!";
                }
                else
                {
                    lbl_progressValue.Text = "Encode Queue Completed!";
                }

                progressBar.Value = 0;

                lbl_source.Text = "-";
                lbl_dest.Text = "-";
                lbl_vEnc.Text = "-";
                lbl_aEnc.Text = "-";
                lbl_title.Text = "-";
                lbl_chapt.Text = "-";

                lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        // Stop's the queue from continuing. 
        private void btn_stop_Click(object sender, EventArgs e)
        {
            cancel = true;
            btn_stop.Visible = false;
            btn_encode.Enabled = true;
            MessageBox.Show("No further items on the queue will start. The current encode process will continue until it is finished. \nClick 'Encode Video' when you wish to continue encoding the queue.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        // Updates the progress bar and progress label for a new status.
        private void updateUIElements()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new ProgressUpdateHandler(updateUIElements));
                    return;
                }

                redrawQueue();

                progressBar.PerformStep();
                lbl_progressValue.Text = string.Format("{0} %", progressBar.Value);
                lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
        }

        // Set's the information lables about the current encode.
        private void setEncValue()
        {
            try
            {
                if (this.InvokeRequired)
                {
                    this.BeginInvoke(new setEncoding(setEncValue));
                }

                // found query is a global varible
                Functions.QueryParser parsed = Functions.QueryParser.Parse(queue.getLastQuery());
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
                lbl_aEnc.Text = parsed.AudioEncoder1;
            }
            catch (Exception)
            {
                // Do Nothing
            }
        }

        // Move an item up the Queue
        private void btn_up_Click(object sender, EventArgs e)
        {
            if (list_queue.SelectedIndices.Count != 0)
            {
                int selected = list_queue.SelectedIndices[0];

                queue.moveUp(selected);
                queue.write2disk("hb_queue_recovery.dat"); // Update the queue recovery file
                redrawQueue();

                if (selected - 1 > 0) 
                    list_queue.Items[selected -1].Selected = true;

                list_queue.Select();
            }
        }

        // Move an item down the Queue
        private void btn_down_Click(object sender, EventArgs e)
        {
            if (list_queue.SelectedIndices.Count != 0)
            {
                int selected = list_queue.SelectedIndices[0];

                queue.moveDown(list_queue.SelectedIndices[0]);
                queue.write2disk("hb_queue_recovery.dat"); // Update the queue recovery file
                redrawQueue();

                if (selected +1 < list_queue.Items.Count) 
                    list_queue.Items[selected + 1].Selected = true;

                list_queue.Select();
            }
        }

        // Remove an item from the queue
        private void btn_delete_Click(object sender, EventArgs e)
        {
            if (list_queue.SelectedIndices.Count != 0)
            {
                queue.remove(list_queue.SelectedIndices[0]);
                queue.write2disk("hb_queue_recovery.dat"); // Update the queue recovery file
                redrawQueue();
                lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
            }
        }

        // Generate a Saveable batch script on the users request
        private void mnu_batch_Click(object sender, EventArgs e)
        {
            SaveFile.FileName = "";
            SaveFile.Filter = "Batch|.bat";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.writeBatchScript(SaveFile.FileName);
        }

        // Export the HandBrake Queue to a file.
        private void mnu_export_Click(object sender, EventArgs e)
        {
            SaveFile.FileName = "";
            SaveFile.Filter = "HandBrake Queue|*.queue";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.write2disk(SaveFile.FileName);
        }

        // Import an exported queue
        private void mnu_import_Click(object sender, EventArgs e)
        {
            OpenFile.FileName = "";
            OpenFile.ShowDialog();
            if (OpenFile.FileName != String.Empty)
                queue.recoverQueue(OpenFile.FileName);
            redrawQueue();
        }

        // Delete a selected item on the queue, if the delete key is pressed.
        private void list_queue_deleteKey(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                if (list_queue.SelectedIndices.Count != 0)
                {
                    queue.remove(list_queue.SelectedIndices[0]);
                    queue.write2disk("hb_queue_recovery.dat"); // Update the queue recovery file
                    redrawQueue();
                }
            }
        }

        // Hide's the window from the users view.
        private void btn_Close_Click(object sender, EventArgs e)
        {
            this.Hide();
        }

        // Hide's the window when the user tries to "x" out of the window instead of closing it.
        protected override void OnClosing(CancelEventArgs e)
        {
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }


    }
}