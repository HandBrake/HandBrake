/*  frmQueue.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.IO;
    using System.Linq;
    using System.Windows.Forms;
    using Functions;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;

    using Model;

    using AudioEncoder = HandBrake.ApplicationServices.Model.Encoding.AudioEncoder;

    /// <summary>
    /// The Queue Window
    /// </summary>
    public partial class frmQueue : Form
    {
        /// <summary>
        /// Update Handler Delegate
        /// </summary>
        private delegate void UpdateHandler();

        /// <summary>
        /// An instance of the Queue service
        /// </summary>
        private readonly IQueueProcessor queue;

        /// <summary>
        /// A reference to the main application window
        /// </summary>
        private readonly frmMain mainWindow;

        /// <summary>
        /// The User setting service
        /// </summary>
        private readonly IUserSettingService userSettingService = new UserSettingService();

        /// <summary>
        /// Initializes a new instance of the <see cref="frmQueue"/> class.
        /// </summary>
        /// <param name="q">
        /// An instance of the queue service.
        /// </param>
        /// <param name="mw">
        /// The main window.
        /// </param>
        public frmQueue(IQueueProcessor q, frmMain mw)
        {
            InitializeComponent();

            this.mainWindow = mw;

            this.queue = q;
            queue.EncodeService.EncodeStarted += this.QueueOnEncodeStart;
            queue.QueueCompleted += this.QueueOnQueueFinished;
            queue.QueuePaused += this.QueueOnPaused;
            queue.QueueManager.QueueChanged += this.queue_QueueListChanged;

            queue.EncodeService.EncodeStarted += this.queue_EncodeStarted;
            queue.EncodeService.EncodeCompleted += this.queue_EncodeEnded;

            drp_completeOption.Text = userSettingService.GetUserSettingString(UserSettingConstants.WhenCompleteAction);
        }

        /// <summary>
        /// Queue Changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void queue_QueueListChanged(object sender, EventArgs e)
        {
            UpdateUiElementsOnQueueChange();
        }

        /// <summary>
        /// Encode Ended
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void queue_EncodeEnded(object sender, EventArgs e)
        {
            queue.EncodeService.EncodeStatusChanged -= EncodeQueue_EncodeStatusChanged;
            ResetEncodeText();
        }

        /// <summary>
        /// Queue Started
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void queue_EncodeStarted(object sender, EventArgs e)
        {
            this.SetCurrentEncodeInformation();
            queue.EncodeService.EncodeStatusChanged += EncodeQueue_EncodeStatusChanged;
        }

        /// <summary>
        /// Display the Encode Status
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void EncodeQueue_EncodeStatusChanged(object sender, HandBrake.ApplicationServices.EventArgs.EncodeProgressEventArgs e)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new EncodeProgessStatus(EncodeQueue_EncodeStatusChanged), new[] { sender, e });
                return;
            }

            this.queue.QueueManager.LastProcessedJob.ElaspedEncodeTime = e.ElapsedTime;

            lbl_encodeStatus.Text =
                string.Format(
                "Encoding: Pass {0} of {1},  {2:00.00}%, FPS: {3:000.0},  Avg FPS: {4:000.0},  Time Remaining: {5},  Elapsed: {6:hh\\:mm\\:ss}",
                e.Task,
                e.TaskCount,
                e.PercentComplete,
                e.CurrentFrameRate,
                e.AverageFrameRate,
                e.EstimatedTimeLeft,
                e.ElapsedTime);

            UpdateStatusLabel();
        }

        /// <summary>
        /// Handle the Queue Paused event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void QueueOnPaused(object sender, EventArgs e)
        {
            SetUiEncodeFinished();
            UpdateUiElementsOnQueueChange();
        }

        /// <summary>
        /// Handle the Queue Finished event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void QueueOnQueueFinished(object sender, EventArgs e)
        {
            SetUiEncodeFinished();
            ResetQueue(); // Reset the Queue Window
        }

        /// <summary>
        /// Handle the Encode Started event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void QueueOnEncodeStart(object sender, EventArgs e)
        {
            SetUiEncodeStarted(); // make sure the UI is set correctly
            UpdateUiElementsOnQueueChange(); // Redraw the Queue, a new encode has started.
        }

        /// <summary>
        /// Initializes the Queue list with the Arraylist from the Queue class
        /// </summary>
        public void SetQueue()
        {
            UpdateUiElementsOnQueueChange();
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
            if (doSetQueue) SetQueue();
            base.Show();
        }

        /// <summary>
        /// Handle the Encode button Click event
        /// </summary>
        /// <param name="sender">The sender</param>
        /// <param name="e">the EventArgs</param>
        private void BtnEncodeClick(object sender, EventArgs e)
        {
            if (!queue.IsProcessing)
            {
                SetUiEncodeStarted();
            }

            lbl_encodeStatus.Text = "Encoding ...";
            queue.Start();
        }

        /// <summary>
        /// Handle the Pause button click event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void BtnPauseClick(object sender, EventArgs e)
        {
            queue.Pause();

            MessageBox.Show(
                "No further items on the queue will start. The current encode process will continue until it is finished. \nClick 'Encode' when you wish to continue encoding the queue.",
                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        // UI Work

        /// <summary>
        /// Setup the UI to show that an encode has started
        /// </summary>
        private void SetUiEncodeStarted()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(SetUiEncodeStarted));
                return;
            }
            btn_encode.Enabled = false;
            btn_pause.Visible = true;
        }

        /// <summary>
        /// Setup the UI to indicate that an encode has finished.
        /// </summary>
        private void SetUiEncodeFinished()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(SetUiEncodeFinished));
                return;
            }
            btn_pause.Visible = false;
            btn_encode.Enabled = true;
        }

        /// <summary>
        /// Reset the Queue Window display
        /// </summary>
        private void ResetQueue()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(ResetQueue));
                return;
            }
            btn_pause.Visible = false;
            btn_encode.Enabled = true;

            this.RedrawQueue();
            ResetEncodeText();
        }

        /// <summary>
        /// Reset the current job text
        /// </summary>
        private void ResetEncodeText()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(ResetEncodeText));
                return;
            }
            lbl_encodeStatus.Text = "Ready";

            lbl_source.Text = "-";
            lbl_dest.Text = "-";
            lbl_encodeOptions.Text = "-";

            UpdateStatusLabel();
        }

        /// <summary>
        /// Redraw the Queue window with the latest information about HandBrakes status
        /// </summary>
        private void RedrawQueue()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(RedrawQueue));
                return;
            }

            list_queue.Items.Clear();
            ReadOnlyCollection<QueueTask> theQueue = queue.QueueManager.Queue;
            foreach (QueueTask queueItem in theQueue)
            {
                string qItem = queueItem.Query;
                EncodeTask parsed = QueryParserUtility.Parse(qItem);

                // Get the DVD Title
                string title = parsed.Title == 0 ? "Auto" : parsed.Title.ToString();

                // Get the DVD Chapters
                string chapters;
                if (parsed.StartPoint == 0)
                    chapters = "Auto";
                else
                {
                    chapters = parsed.StartPoint.ToString();
                    if (parsed.EndPoint != 0)
                        chapters = chapters + " - " + parsed.EndPoint;
                }

                ListViewItem item = new ListViewItem
                    { Tag = queueItem, Text = EnumHelper<QueueItemStatus>.GetDescription(queueItem.Status) };
                item.SubItems.Add(title);
                item.SubItems.Add(chapters); // Chapters
                item.SubItems.Add(queueItem.Source); // Source
                item.SubItems.Add(queueItem.Destination); // Destination
                item.SubItems.Add(EnumHelper<VideoEncoder>.GetDisplayValue(parsed.VideoEncoder));

                // Display The Audio Track Information
                string audio = string.Empty;
                foreach (AudioTrack track in parsed.AudioTracks)
                {
                    if (audio != string.Empty)
                        audio += ", " + EnumHelper<AudioEncoder>.GetDescription(track.Encoder);
                    else
                        audio = EnumHelper<AudioEncoder>.GetDescription(track.Encoder);
                }
                item.SubItems.Add(audio); // Audio

                list_queue.Items.Add(item);
            }
        }

        /// <summary>
        /// Update the UI elements
        /// </summary>
        private void UpdateUiElementsOnQueueChange()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(UpdateUiElementsOnQueueChange));
                return;
            }

            RedrawQueue();
            UpdateStatusLabel();
        }

        /// <summary>
        /// Update the Display
        /// </summary>
        private void UpdateStatusLabel()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(UpdateStatusLabel));
                return;
            }

            lbl_encodesPending.Text = string.Format("{0} encodes(s) pending", this.queue.QueueManager.Count);
        }

        /// <summary>
        /// Set the window up with the current encode information
        /// </summary>
        private void SetCurrentEncodeInformation()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateHandler(SetCurrentEncodeInformation));
                }

                EncodeTask parsed = QueryParserUtility.Parse(queue.QueueManager.LastProcessedJob.Query);

                // Get title and chapters
                string title = parsed.Title == 0 ? "Auto" : parsed.Title.ToString();
                string chapterlbl;
                if (Equals(parsed.StartPoint, 0))
                    chapterlbl = "Auto";
                else
                {
                    string chapters = parsed.StartPoint.ToString();
                    if (parsed.EndPoint != 0)
                        chapters = chapters + " - " + parsed.EndPoint;
                    chapterlbl = chapters;
                }

                // Get audio information
                string audio = string.Empty;
                foreach (AudioTrack track in parsed.AudioTracks)
                {
                    if (audio != string.Empty)
                        audio += ", " + EnumHelper<AudioEncoder>.GetDescription(track.Encoder);
                    else
                        audio = EnumHelper<AudioEncoder>.GetDescription(track.Encoder);
                }

                // found query is a global varible        
                lbl_encodeStatus.Text = "Encoding ...";
                lbl_source.Text = queue.QueueManager.LastProcessedJob.Source + "(Title: " + title + " Chapters: " + chapterlbl + ")";
                lbl_dest.Text = queue.QueueManager.LastProcessedJob.Destination;
                lbl_encodeOptions.Text = string.Format("Video: {0},  Audio: {1}\nx264 Options: {2}",
                    EnumHelper<VideoEncoder>.GetDisplayValue(parsed.VideoEncoder),
                    audio, 
                    parsed.AdvancedEncoderOptions);

            }
            catch (Exception)
            {
                // Do Nothing
            }
        }

        /* Right Click Menu */

        /// <summary>
        /// Handle the Move Up Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuUpClick(object sender, EventArgs e)
        {
            MoveUp();
        }

        /// <summary>
        /// Handle the Move down Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuDownClick(object sender, EventArgs e)
        {
            MoveDown();
        }

        /// <summary>
        /// Edit a job
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuEditClick(object sender, EventArgs e)
        {
            if (list_queue.SelectedIndices != null && list_queue.SelectedIndices.Count != 0)
            {
                lock (queue)
                {
                    lock (list_queue)
                    {
                        QueueTask index = list_queue.SelectedItems[0].Tag as QueueTask;
                        mainWindow.RecievingJob(index);
                        queue.QueueManager.Remove(index);
                        RedrawQueue();
                    }
                }
            }
        }

        /// <summary>
        /// Handle the delete Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuDeleteClick(object sender, EventArgs e)
        {
            DeleteSelectedItems();
        }

        /* Keyboard Shortcuts */

        /// <summary>
        /// Handle the delete keyboard press
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ListQueueDeleteKey(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete && e.Modifiers == Keys.None)
                DeleteSelectedItems();
        }

        /* Queue Management */

        /// <summary>
        /// Move items up in the queue
        /// </summary>
        private void MoveUp()
        {
            // If there are selected items and the first item is not selected
            if (list_queue.SelectedIndices.Count > 0 && !list_queue.SelectedIndices.Contains(0))
            {
                // Copy the selected indices to preserve them during the movement
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                // Move up each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.QueueManager.MoveUp(selectedIndex);

                // Keep the selected item(s) selected, now moved up one index
                foreach (int selectedIndex in selectedIndices)
                    if (selectedIndex - 1 > -1) // Defensive programming: ensure index is good
                        list_queue.Items[selectedIndex - 1].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }

        /// <summary>
        /// Move items down in the queue
        /// </summary>
        private void MoveDown()
        {
            // If there are selected items and the last item is not selected
            if (list_queue.SelectedIndices.Count > 0 &&
                !list_queue.SelectedIndices.Contains(list_queue.Items[list_queue.Items.Count - 1].Index))
            {
                // Copy the selected indices to preserve them during the movement
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                // Reverse the indices to move the items down from last to first (preserves indices)
                selectedIndices.Reverse();

                // Move down each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.QueueManager.MoveDown(selectedIndex);

                // Keep the selected item(s) selected, now moved down one index
                foreach (int selectedIndex in selectedIndices)
                    if (selectedIndex + 1 < list_queue.Items.Count) // Defensive programming: ensure index is good
                        list_queue.Items[selectedIndex + 1].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }

        /// <summary>
        /// Delete the currently selected items on the queue
        /// </summary>
        private void DeleteSelectedItems()
        {
            // If there are selected items
            if (list_queue.SelectedIndices.Count > 0)
            {
                // Remove each selected item
                foreach (ListViewItem selectedIndex in this.list_queue.SelectedItems)
                    queue.QueueManager.Remove((QueueTask)selectedIndex.Tag);

                // Select the first item after deletion.
                if (list_queue.Items.Count > 0)
                    list_queue.Items[0].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }

        /* Queue Import / Export features */

        /// <summary>
        /// Create a batch script
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuBatchClick(object sender, EventArgs e)
        {
            SaveFile.FileName = string.Empty;
            SaveFile.Filter = "Batch|.bat";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.QueueManager.WriteBatchScriptToFile(SaveFile.FileName);
        }

        /// <summary>
        /// Export Queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuExportClick(object sender, EventArgs e)
        {
            SaveFile.FileName = string.Empty;
            SaveFile.Filter = "HandBrake Queue|*.queue";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.QueueManager.BackupQueue(SaveFile.FileName);
        }

        /// <summary>
        /// Import Queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuImportClick(object sender, EventArgs e)
        {
            OpenFile.FileName = string.Empty;
            OpenFile.ShowDialog();
            if (OpenFile.FileName != String.Empty)
                queue.QueueManager.RestoreQueue(OpenFile.FileName);
        }

        /// <summary>
        /// Readd current job to queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuReaddClick(object sender, EventArgs e)
        {
            if (queue.QueueManager.LastProcessedJob != null && !queue.QueueManager.LastProcessedJob.IsEmpty)
            {
                queue.QueueManager.Add(queue.QueueManager.LastProcessedJob);
            }
        }

        /* Overrides */

        /// <summary>
        /// Hide's the window when the user tries to "x" out of the window instead of closing it.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected override void OnClosing(CancelEventArgs e)
        {
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }

        /// <summary>
        /// Change the OnComplete option setting.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void CompleteOptionChanged(object sender, EventArgs e)
        {
            userSettingService.SetUserSetting(UserSettingConstants.WhenCompleteAction, drp_completeOption.Text);
        }

        /// <summary>
        /// Clear all completed items
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnuClearCompleted_Click(object sender, EventArgs e)
        {
            this.queue.QueueManager.ClearCompleted();
        }

        /// <summary>
        /// Retry Job Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void mnu_Retry_Click(object sender, EventArgs e)
        {
            if (list_queue.SelectedIndices.Count != 0)
            {
                lock (queue)
                {
                    lock (list_queue)
                    {
                        QueueTask index = list_queue.SelectedItems[0].Tag as QueueTask;

                        try
                        {
                            queue.QueueManager.ResetJobStatusToWaiting(index);
                        } 
                        catch (Exception)
                        {
                            MessageBox.Show(
                                "Can only retry a job if it is in an Error or Completed state.",
                                "Notice",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Information);
                        }
                        RedrawQueue();
                    }
                }
            }
        }
    }
}