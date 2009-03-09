/*  frmQueue.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmQueue : Form
    {
        private delegate void UpdateHandler();
        Queue.QueueHandler queue;

        public frmQueue(Queue.QueueHandler q)
        {
            InitializeComponent();

            this.queue = q;
            queue.OnEncodeStart += new EventHandler(queue_OnEncodeStart);
            queue.OnQueueFinished += new EventHandler(queue_OnQueueFinished);
            queue.OnPaused += new EventHandler(queue_OnPaused);
        }
        void queue_OnPaused(object sender, EventArgs e)
        {
            setUIEncodeFinished();
            updateUIElements();
        }
        void queue_OnQueueFinished(object sender, EventArgs e)
        {
            setUIEncodeFinished();
            resetQueue(); // Reset the Queue Window
        }
        void queue_OnEncodeStart(object sender, EventArgs e)
        {
            setUIEncodeStarted(); // make sure the UI is set correctly
            setCurrentEncodeInformation();
            updateUIElements(); // Redraw the Queue, a new encode has started.
        }

        /// <summary>
        /// Initializes the Queue list with the Arraylist from the Queue class
        /// </summary>
        public void setQueue()
        {
            updateUIElements();
        }

        /// <summary>
        /// Initializes the Queue list, then shows and activates the window
        /// </summary>
        public new void Show()
        {
            Show(true);
        }

        /// <summary>
        /// Initializes the Queue list only if doSetQueue is true, then shows and activates the window
        /// </summary>
        /// <param name="doSetQueue">Indicates whether to call setQueue() before showing the window</param>
        public void Show(bool doSetQueue)
        {
            if (doSetQueue) setQueue();
            base.Show();
            Activate();
        }

        // Start and Stop Controls
        private void btn_encode_Click(object sender, EventArgs e)
        {
            if (queue.isPaused)
            {
                setUIEncodeStarted();
                MessageBox.Show("Encoding restarted", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }

            if (!queue.isEncodeStarted)
                queue.startEncode();

        }
        private void btn_pause_Click(object sender, EventArgs e)
        {
            queue.pauseEncode();
            setUIEncodeFinished();
            resetQueue();
            MessageBox.Show("No further items on the queue will start. The current encode process will continue until it is finished. \nClick 'Encode Video' when you wish to continue encoding the queue.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        // Window Display Management
        private void setUIEncodeStarted()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(setUIEncodeStarted));
                return;
            }
            btn_encode.Enabled = false;
            btn_pause.Visible = true;
        }
        private void setUIEncodeFinished()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(setUIEncodeFinished));
                return;
            }
            btn_pause.Visible = false;
            btn_encode.Enabled = true;
        }
        private void resetQueue()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(resetQueue));
                return;
            }
            btn_pause.Visible = false;
            btn_encode.Enabled = true;

            lbl_source.Text = "-";
            lbl_dest.Text = "-";
            lbl_vEnc.Text = "-";
            lbl_aEnc.Text = "-";
            lbl_title.Text = "-";
            lbl_chapt.Text = "-";

            lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
        }
        private void redrawQueue()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(redrawQueue));
                return;
            }

            list_queue.Items.Clear();
            List<Queue.QueueItem> theQueue = queue.getQueue();
            foreach (Queue.QueueItem queue_item in theQueue)
            {
                string q_item = queue_item.Query;
                Functions.QueryParser parsed = Functions.QueryParser.Parse(q_item);

                // Get the DVD Title
                 string title = parsed.DVDTitle == 0 ? "Auto" : parsed.DVDTitle.ToString();

                // Get the DVD Chapters
                string chapters;
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
                item.SubItems.Add(queue_item.Source); // Source
                item.SubItems.Add(queue_item.Destination); // Destination
                item.SubItems.Add(parsed.VideoEncoder); // Video

                // Display the first 4 audio tracks.
                String audio = parsed.AudioEncoder1;
                if (parsed.AudioEncoder2 != null)
                    audio += ", " + parsed.AudioEncoder2;

                if (parsed.AudioEncoder3 != null)
                    audio += ", " + parsed.AudioEncoder3;

                if (parsed.AudioEncoder4 != null)
                    audio += ", " + parsed.AudioEncoder4;

                item.SubItems.Add(audio); // Audio

                list_queue.Items.Add(item);
            }
        }
        private void updateUIElements()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(updateUIElements));
                return;
            }

            redrawQueue();
            lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
        }
        private void setCurrentEncodeInformation()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateHandler(setCurrentEncodeInformation));
                }

                // found query is a global varible
                Functions.QueryParser parsed = Functions.QueryParser.Parse(queue.getLastQueryItem().Query);
                lbl_source.Text = queue.getLastQueryItem().Source;
                lbl_dest.Text = queue.getLastQueryItem().Destination;

                lbl_title.Text = parsed.DVDTitle == 0 ? "Auto" : parsed.DVDTitle.ToString();

                if (Equals(parsed.DVDChapterStart, 0))
                  lbl_chapt.Text = "Auto";
                else
                {
                    string chapters = parsed.DVDChapterStart.ToString();
                    if (parsed.DVDChapterFinish != 0)
                        chapters = chapters + " - " + parsed.DVDChapterFinish;
                    lbl_chapt.Text = chapters;
                }

                lbl_vEnc.Text = parsed.VideoEncoder;
                String audio = parsed.AudioEncoder1;
                if (parsed.AudioEncoder2 != null)
                    audio += ", " + parsed.AudioEncoder2;

                if (parsed.AudioEncoder3 != null)
                    audio += ", " + parsed.AudioEncoder3;

                if (parsed.AudioEncoder4 != null)
                    audio += ", " + parsed.AudioEncoder4;

                lbl_aEnc.Text = audio;
            }
            catch (Exception)
            {
                // Do Nothing
            }
        }
        private void deleteSelectedItems()
        {
            // If there are selected items
            if (list_queue.SelectedIndices.Count > 0)
            {
                // Save the selected indices to select them after the move
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                int firstSelectedIndex = selectedIndices[0];

                // Reverse the list to delete the items from last to first (preserves indices)
                selectedIndices.Reverse();
                
                // Remove each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.remove(selectedIndex);

                queue.write2disk("hb_queue_recovery.xml"); // Update the queue recovery file
                updateUIElements();

                // Select the item where the first deleted item was previously
                if (firstSelectedIndex < list_queue.Items.Count) 
                    list_queue.Items[firstSelectedIndex].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }

        // Queue Management
        private void btn_up_Click(object sender, EventArgs e)
        {
            // If there are selected items and the first item is not selected
            if (list_queue.SelectedIndices.Count > 0 && ! list_queue.SelectedIndices.Contains(0))
            {
                // Copy the selected indices to preserve them during the movement
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                // Move up each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.moveUp(selectedIndex);

                queue.write2disk("hb_queue_recovery.xml"); // Update the queue recovery file
                updateUIElements();

                // Keep the selected item(s) selected, now moved up one index
                foreach (int selectedIndex in selectedIndices)
                    if (selectedIndex - 1 > -1) // Defensive programming: ensure index is good
                        list_queue.Items[selectedIndex - 1].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }
        private void btn_down_Click(object sender, EventArgs e)
        {
            // If there are selected items and the last item is not selected
            if (list_queue.SelectedIndices.Count > 0 && 
                ! list_queue.SelectedIndices.Contains(list_queue.Items[list_queue.Items.Count-1].Index))
            {
                // Copy the selected indices to preserve them during the movement
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                // Reverse the indices to move the items down from last to first (preserves indices)
                selectedIndices.Reverse();

                // Move down each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.moveDown(selectedIndex);
                
                queue.write2disk("hb_queue_recovery.xml"); // Update the queue recovery file
                updateUIElements();

                // Keep the selected item(s) selected, now moved down one index
                foreach (int selectedIndex in selectedIndices)
                    if (selectedIndex + 1 < list_queue.Items.Count) // Defensive programming: ensure index is good
                        list_queue.Items[selectedIndex + 1].Selected = true; 
            }

            list_queue.Select(); // Activate the control to show the selected items
        }
        private void btn_delete_Click(object sender, EventArgs e)
        {
            deleteSelectedItems();
        }
        private void list_queue_deleteKey(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
                deleteSelectedItems();
        }

        // Queue Import/Export Features
        private void mnu_batch_Click(object sender, EventArgs e)
        {
            SaveFile.FileName = "";
            SaveFile.Filter = "Batch|.bat";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.writeBatchScript(SaveFile.FileName);
        }
        private void mnu_export_Click(object sender, EventArgs e)
        {
            SaveFile.FileName = "";
            SaveFile.Filter = "HandBrake Queue|*.queue";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.write2disk(SaveFile.FileName);
        }
        private void mnu_import_Click(object sender, EventArgs e)
        {
            OpenFile.FileName = "";
            OpenFile.ShowDialog();
            if (OpenFile.FileName != String.Empty)
                queue.recoverQueue(OpenFile.FileName);
            updateUIElements();
        }
        private void mnu_readd_Click(object sender, EventArgs e)
        {
            if (queue.getLastQueryItem() != null)
            {
                queue.add(queue.getLastQueryItem().Query, queue.getLastQueryItem().Source, queue.getLastQueryItem().Destination);
                queue.write2disk("hb_queue_recovery.xml"); // Update the queue recovery file
                updateUIElements();
            }
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
