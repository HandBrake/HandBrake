/*  AudioPanel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System;
    using System.Collections;
    using System.Linq;
    using System.Windows.Forms;
    using Functions;

    using HandBrake.ApplicationServices.Parsing;

    using Presets;
    using AudioTrack = Model.AudioTrack;

    /// <summary>
    /// The AudioPanel Control
    /// </summary>
    public partial class AudioPanel : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AudioPanel"/> class. 
        /// Create a new instance of the Audio Panel
        /// </summary>
        public AudioPanel()
        {
            InitializeComponent();
            drp_audioMix.SelectedItem = "Dolby Pro Logic II";
            drp_audioSample.SelectedIndex = 1;
        }

        /// <summary>
        /// The audio list has changed
        /// </summary>
        public event EventHandler AudioListChanged;

        /// <summary>
        /// Get the audio panel
        /// </summary>
        /// <returns>A listview containing the audio tracks</returns>
        public DataGridView GetAudioPanel()
        {
            return audioList;
        }

        /// <summary>
        /// Set the File Container. This funciton is used to limit the available options for each file container.
        /// </summary>
        /// <param name="path">
        /// the file path
        /// </param>
        public void SetContainer(string path)
        {
            string oldval = drp_audioEncoder.Text;
            if ((path.Contains("MP4")) || (path.Contains("M4V")))
            {
                drp_audioEncoder.Items.Clear();
                drp_audioEncoder.Items.Add("AAC (faac)");
                drp_audioEncoder.Items.Add("MP3 (lame)");
                drp_audioEncoder.Items.Add("AC3 Passthru");
                if ((oldval != "AAC (faac)") && (oldval != "AC3 Passthru"))
                    drp_audioEncoder.SelectedIndex = 0;
                else
                    drp_audioEncoder.SelectedItem = oldval;
            }
            else if (path.Contains("MKV"))
            {
                drp_audioEncoder.Items.Clear();
                drp_audioEncoder.Items.Add("AAC (faac)");
                drp_audioEncoder.Items.Add("MP3 (lame)");
                drp_audioEncoder.Items.Add("AC3 Passthru");
                drp_audioEncoder.Items.Add("DTS Passthru");
                drp_audioEncoder.Items.Add("Vorbis (vorbis)");
                drp_audioEncoder.SelectedItem = oldval;

                if (drp_audioEncoder.Text == string.Empty)
                    drp_audioEncoder.SelectedIndex = 0;
            }

            // Make sure the table is updated with new audio codecs
            foreach (DataGridViewRow row in audioList.Rows)
            {
                if (!drp_audioEncoder.Items.Contains(row.Cells[2].Value))
                    row.Cells[2].Value = drp_audioEncoder.Items[0].ToString();
            }
        }

        /// <summary>
        /// Checks if the settings used required the .m4v (rather than .mp4) extension
        /// </summary>
        /// <returns>True if m4v is required</returns>
        public bool RequiresM4V()
        {
            return this.audioList.Rows.Cast<DataGridViewRow>().Any(row => row.Cells[2].Value.ToString().Contains("AC3"));
        }

        /// <summary>
        /// Load an arraylist of AudioTrack items into the list.
        /// </summary>
        /// <param name="audioTracks">List of audio tracks</param>
        public void LoadTracks(ArrayList audioTracks)
        {
            ClearAudioList();

            if (audioTracks == null)
                return;

            foreach (AudioTrack track in audioTracks)
            {
                DataGridViewRow newTrack = new DataGridViewRow();
                newTrack.CreateCells(audioList);
                newTrack.Cells[0].Value = GetNewID().ToString();
                newTrack.Cells[1].Value = "Automatic";
                newTrack.Cells[2].Value = track.Encoder;
                newTrack.Cells[3].Value = track.MixDown;
                newTrack.Cells[4].Value = track.SampleRate;
                newTrack.Cells[5].Value = track.Encoder.Contains("AC3") ? "Auto" : track.Bitrate;
                newTrack.Cells[6].Value = track.DRC;
                AddTrackForPreset(newTrack);
            }
        }

        /// <summary>
        /// Set the Track list dropdown from the parsed title captured during the scan
        /// </summary>
        /// <param name="selectedTitle">The selected title</param>
        /// <param name="preset">A preset</param>
        public void SetTrackList(Title selectedTitle, Preset preset)
        {
            if (selectedTitle.AudioTracks.Count == 0)
            {
                audioList.Rows.Clear();
                drp_audioTrack.Items.Clear();
                drp_audioTrack.Items.Add("None Found");
                drp_audioTrack.SelectedIndex = 0;
                return;
            }

            // The Source Information for the title will have changed, so set all the tracks to Automatic.
            foreach (DataGridViewRow row in this.audioList.Rows)
            {
                row.Cells[1].Value = "Automatic";
            }

            // Setup the Audio track source dropdown with the new audio tracks.
            drp_audioTrack.Items.Clear();
            drp_audioTrack.Items.Add("Automatic");
            drp_audioTrack.Items.AddRange(selectedTitle.AudioTracks.ToArray());

            // Re-add any audio tracks that the preset has.
            if (audioList.Rows.Count == 0 && preset != null)
            {
                QueryParser parsed = QueryParser.Parse(preset.Query);
                foreach (AudioTrack audioTrack in parsed.AudioInformation)
                {
                    DataGridViewRow newTrack = new DataGridViewRow();
                    newTrack.CreateCells(audioList);
                    newTrack.Cells[0].Value = GetNewID().ToString();
                    newTrack.Cells[1].Value = (audioTrack.Track);
                    newTrack.Cells[2].Value = (audioTrack.Encoder);
                    newTrack.Cells[3].Value = (audioTrack.MixDown);
                    newTrack.Cells[4].Value = (audioTrack.SampleRate);
                    newTrack.Cells[5].Value = (audioTrack.Bitrate);
                    newTrack.Cells[6].Value = (audioTrack.DRC);
                    audioList.Rows.Add(newTrack);
                }
            }

            // Handle Native Language and "Dub Foreign language audio" and "Use Foreign language audio and Subtitles" Options
            if (Properties.Settings.Default.NativeLanguage == "Any")
                drp_audioTrack.SelectedIndex = drp_audioTrack.Items.Count >= 2 ? 1 : 0;
            else
            {
                if (Properties.Settings.Default.DubMode > 1) // "Dub Foreign language audio" 
                {
                    int i = 0;
                    foreach (object item in drp_audioTrack.Items)
                    {
                        if (item.ToString().Contains(Properties.Settings.Default.NativeLanguage))
                        {
                            drp_audioTrack.SelectedIndex = i;
                            break;
                        }

                        i++;
                    }

                    if (drp_audioTrack.SelectedItem != null)
                        foreach (DataGridViewRow item in audioList.Rows)
                            item.Cells[1].Value = drp_audioTrack.SelectedItem.ToString();
                    else
                    {
                        drp_audioTrack.SelectedIndex = 0;
                        if (drp_audioTrack.SelectedItem != null)
                            foreach (DataGridViewRow item in audioList.Rows)
                                item.Cells[1].Value = drp_audioTrack.SelectedItem.ToString();
                    }
                }
                else
                    drp_audioTrack.SelectedIndex = drp_audioTrack.Items.Count >= 3 ? 2 : 0;
                // "Use Foreign language audio and Subtitles"
            }
        }

        // Control and ListView

        /// <summary>
        /// one of the controls has changed. Event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void controlChanged(object sender, EventArgs e)
        {
            Control ctl = (Control)sender;

            switch (ctl.Name)
            {
                case "drp_audioTrack":
                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
                        audioList.SelectedRows[0].Cells[1].Value = drp_audioTrack.Text;
                    break;
                case "drp_audioEncoder":
                    SetMixDown();
                    SetBitrate();

                    // Configure the widgets with values
                    if (drp_audioEncoder.Text.Contains("AC3") || drp_audioEncoder.Text.Contains("DTS"))
                    {
                        drp_audioMix.Enabled =
                            drp_audioBitrate.Enabled = drp_audioSample.Enabled = tb_drc.Enabled = false;
                        lbl_bitrate.Enabled =
                            lbl_drc.Enabled =
                            lbl_drcHeader.Enabled = lbl_mixdown.Enabled = lbl_sampleRate.Enabled = false;
                    }
                    else
                    {
                        drp_audioMix.Enabled =
                            drp_audioBitrate.Enabled = drp_audioSample.Enabled = tb_drc.Enabled = true;
                        lbl_bitrate.Enabled =
                            lbl_drc.Enabled =
                            lbl_drcHeader.Enabled = lbl_mixdown.Enabled = lbl_sampleRate.Enabled = true;
                    }

                    // Update an item in the Audio list if required.
                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
                        audioList.SelectedRows[0].Cells[2].Value = drp_audioEncoder.Text;
                    break;
                case "drp_audioMix":
                    SetBitrate();

                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
                        audioList.SelectedRows[0].Cells[3].Value = drp_audioMix.Text;
                    break;
                case "drp_audioSample":
                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
                        audioList.SelectedRows[0].Cells[4].Value = drp_audioSample.Text;
                    break;
                case "drp_audioBitrate":
                    // Update an item in the Audio list if required.
                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
                        audioList.SelectedRows[0].Cells[5].Value = drp_audioBitrate.Text;
                    break;
                case "tb_drc":
                    double value;
                    if (tb_drc.Value == 0) value = 0;
                    else
                        value = ((tb_drc.Value - 1) / 10.0) + 1;

                    lbl_drc.Text = value.ToString();

                    // Update an item in the Audio list if required.
                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
                    {
                        audioList.SelectedRows[0].Cells[6].Value = value.ToString();
                        audioList.Select();
                    }
                    break;
            }

            audioList.Select();
        }

        /// <summary>
        /// The Audio List selected index changed event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void audioList_SelectionChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the Audio List.
            if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
            {
                drp_audioTrack.SelectedItem = audioList.SelectedRows[0].Cells[1].Value;
                drp_audioEncoder.SelectedItem = audioList.SelectedRows[0].Cells[2].Value;
                drp_audioMix.SelectedItem = audioList.SelectedRows[0].Cells[3].Value;
                drp_audioSample.SelectedItem = audioList.SelectedRows[0].Cells[4].Value;
                drp_audioBitrate.SelectedItem = audioList.SelectedRows[0].Cells[5].Value;
                double drcValue;
                int drcCalculated;
                double.TryParse(audioList.SelectedRows[0].Cells[6].Value.ToString(), out drcValue);
                if (drcValue != 0)
                    drcValue = ((drcValue * 10) + 1) - 10;
                int.TryParse(drcValue.ToString(), out drcCalculated);
                tb_drc.Value = drcCalculated;
                lbl_drc.Text = audioList.SelectedRows[0].Cells[6].Value.ToString();

                AudioTrackGroup.Text = "Selected Track: " + audioList.SelectedRows[0].Cells[0].Value;
            }
            else
                AudioTrackGroup.Text = "Selected Track: None (Click \"Add Track\" to add)";
        }

        // Track Controls

        /// <summary>
        /// The Add Audio Track button event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_addAudioTrack_Click(object sender, EventArgs e)
        {
            if (drp_audioTrack.Text == "None Found")
            {
                MessageBox.Show(
                    "Your source appears to have no audio tracks that HandBrake supports.",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                return;
            }

            double value = 0;
            if (tb_drc.Value != 0)
                value = ((tb_drc.Value - 1) / 10.0) + 1;

            // Create a new row for the Audio list based on the currently selected items in the dropdown.
            DataGridViewRow newTrack = new DataGridViewRow();
            newTrack.CreateCells(audioList);
            newTrack.Cells[0].Value = GetNewID().ToString();
            newTrack.Cells[1].Value = drp_audioTrack.Text;
            newTrack.Cells[2].Value = drp_audioEncoder.Text;
            newTrack.Cells[3].Value = drp_audioMix.Text;
            newTrack.Cells[4].Value = drp_audioSample.Text;
            newTrack.Cells[5].Value = drp_audioBitrate.Text;
            newTrack.Cells[6].Value = value.ToString();
            audioList.Rows.Add(newTrack);

            // The Audio List has changed to raise the event.
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());

            // Select the newly added track and select the control       
            audioList.ClearSelection();
            audioList.Rows[audioList.Rows.Count - 1].Selected = true;
            audioList.Select();
        }

        /// <summary>
        /// The Remove Track button event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void btn_RemoveAudioTrack_Click(object sender, EventArgs e)
        {
            RemoveTrack();
        }

        // Audio List Menu

        /// <summary>
        /// The Audio List Move Up menu item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void audioList_moveup_Click(object sender, EventArgs e)
        {
            MoveTrack(true);
        }

        /// <summary>
        /// The audio list move down menu item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void audioList_movedown_Click(object sender, EventArgs e)
        {
            MoveTrack(false);
        }

        /// <summary>
        /// The audio list remove menu item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void audioList_remove_Click(object sender, EventArgs e)
        {
            RemoveTrack();
        }

        // Private Functions

        /// <summary>
        /// Add track for preset
        /// </summary>
        /// <param name="item">
        /// The item.
        /// </param>
        private void AddTrackForPreset(DataGridViewRow item)
        {
            audioList.Rows.Add(item);
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
        }

        /// <summary>
        /// Clear the audio list
        /// </summary>
        private void ClearAudioList()
        {
            audioList.Rows.Clear();
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
        }

        /// <summary>
        /// Get a new ID for the next audio track
        /// </summary>
        /// <returns>
        /// an int
        /// </returns>
        private int GetNewID()
        {
            return audioList.Rows.Count + 1;
        }

        /// <summary>
        /// Remove an audio track from the list
        /// </summary>
        private void RemoveTrack()
        {
            // Remove the Item and reselect the control if the following conditions are met.
            if (audioList.SelectedRows.Count != 0)
            {
                // The Audio List is about to change so raise the event.
                if (this.AudioListChanged != null)
                    this.AudioListChanged(this, new EventArgs());

                // Record the current selected index.
                int currentPosition = audioList.SelectedRows[0].Index;

                audioList.Rows.Remove(audioList.SelectedRows[0]);

                // Now reslect the correct item and give focus to the audio list.
                if (audioList.Rows.Count != 0)
                {
                    audioList.ClearSelection();
                    if (currentPosition <= (audioList.Rows.Count - 1))
                        audioList.Rows[currentPosition].Selected = true;
                    else if (currentPosition > (audioList.Rows.Count - 1))
                        audioList.Rows[audioList.Rows.Count - 1].Selected = true;

                    audioList.Select();
                }
                // Regenerate the ID numers
                ReGenerateListIDs();
            }
        }

        /// <summary>
        /// Move an audio track up or down the audio list
        /// </summary>
        /// <param name="up">
        /// The up.
        /// </param>
        private void MoveTrack(bool up)
        {
            if (audioList.SelectedRows.Count == 0) return;

            DataGridViewRow item = audioList.SelectedRows[0];
            int index = item.Index;

            if (up) index--;
            else index++;
    
            if (index < audioList.Rows.Count || (audioList.Rows.Count > index && index >= 0))
            {
                audioList.Rows.Remove(item);
                audioList.Rows.Insert(index, item);
                audioList.ClearSelection();
                item.Selected = true;
            }
        }

        /// <summary>
        /// Regenerate all the audio track id's on the audio list
        /// </summary>
        private void ReGenerateListIDs()
        {
            int i = 1;
            foreach (DataGridViewRow item in audioList.Rows)
            {
                item.Cells[0].Value = i.ToString();
                i++;
            }
        }

        /// <summary>
        /// Set the bitrate dropdown
        /// </summary>
        private void SetBitrate()
        {
            int max = 0;
            drp_audioBitrate.Items.Remove("Auto");
            drp_audioBitrate.Items.Remove("192");
            drp_audioBitrate.Items.Remove("224");
            drp_audioBitrate.Items.Remove("256");
            drp_audioBitrate.Items.Remove("320");
            drp_audioBitrate.Items.Remove("384");
            drp_audioBitrate.Items.Remove("448");
            drp_audioBitrate.Items.Remove("768");

            switch (drp_audioEncoder.Text)
            {
                case "AAC (faac)":
                    max = drp_audioMix.Text.Contains("6 Channel") ? 768 : 320;
                    break;
                case "MP3 (lame)":
                    max = 320;
                    break;
                case "Vorbis (vorbis)":
                    max = 384;
                    break;
                case "AC3 Passthru":
                    drp_audioBitrate.Items.Add("Auto");
                    drp_audioBitrate.SelectedItem = "Auto";
                    drp_audioSample.SelectedItem = "Auto";
                    break;
                case "DTS Passthru":
                    drp_audioBitrate.Items.Add("Auto");
                    drp_audioBitrate.SelectedItem = "Auto";
                    drp_audioSample.SelectedItem = "Auto";
                    break;
            }

            if (max > 160)
            {
                drp_audioBitrate.Items.Add("192");
                drp_audioBitrate.Items.Add("224");
                drp_audioBitrate.Items.Add("256");
                drp_audioBitrate.Items.Add("320");
            }

            if (max > 320)
            {
                drp_audioBitrate.Items.Add("384");
            }

            if (max == 768)
            {
                drp_audioBitrate.Items.Add("448");
                drp_audioBitrate.Items.Add("768");
            }

            if (drp_audioBitrate.SelectedItem == null)
                drp_audioBitrate.SelectedIndex = drp_audioBitrate.Items.Count - 1;
        }

        /// <summary>
        /// Set the mixdown dropdown
        /// </summary>
        private void SetMixDown()
        {
            drp_audioMix.Items.Clear();
            drp_audioMix.Items.Add("Mono");
            drp_audioMix.Items.Add("Stereo");
            drp_audioMix.Items.Add("Dolby Surround");
            drp_audioMix.Items.Add("Dolby Pro Logic II");
            drp_audioMix.Items.Add("6 Channel Discrete");
            drp_audioMix.Items.Add("AC3 Passthru");
            drp_audioMix.Items.Add("DTS Passthru");

            drp_audioMix.SelectedItem = "Dolby Pro Logic II";

            switch (drp_audioEncoder.Text)
            {
                case "AAC (faac)":
                    drp_audioMix.Items.Remove("AC3 Passthru");
                    drp_audioMix.Items.Remove("DTS Passthru");
                    break;
                case "MP3 (lame)":
                    drp_audioMix.Items.Remove("6 Channel Discrete");
                    drp_audioMix.Items.Remove("AC3 Passthru");
                    drp_audioMix.Items.Remove("DTS Passthru");
                    break;
                case "Vorbis (vorbis)":
                    drp_audioMix.Items.Remove("AC3 Passthru");
                    drp_audioMix.Items.Remove("DTS Passthru");
                    break;
                case "AC3 Passthru":
                    drp_audioMix.SelectedItem = "AC3 Passthru";
                    break;
                case "DTS Passthru":
                    drp_audioMix.SelectedItem = "DTS Passthru";
                    break;
            }
        }
    }
}