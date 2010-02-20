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
    using Parsing;
    using AudioTrack = Model.AudioTrack;

    public partial class AudioPanel : UserControl
    {
        /// <summary>
        /// The audio list has changed
        /// </summary>
        public event EventHandler AudioListChanged;

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
        /// Get the audio panel
        /// </summary>
        /// <returns>A listview containing the audio tracks</returns>
        public ListView GetAudioPanel()
        {
            return lv_audioList;
        }

        /// <summary>
        /// Set the File Container. This funciton is used to limit the available options for each file container.
        /// </summary>
        /// <param name="path"></param>
        public void SetContainer(string path)
        {
            string oldval = drp_audioEncoder.Text;
            if ((path.Contains("MP4")) || (path.Contains("M4V")))
            {
                drp_audioEncoder.Items.Clear();
                drp_audioEncoder.Items.Add("AAC (faac)");
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
            foreach (ListViewItem row in lv_audioList.Items)
            {
                if (!drp_audioEncoder.Items.Contains(row.SubItems[2].Text))
                    row.SubItems[2].Text = drp_audioEncoder.Items[0].ToString();
            }
        }

        /// <summary>
        /// Checks if the settings used required the .m4v (rather than .mp4) extension
        /// </summary>
        /// <returns></returns>
        public bool RequiresM4V()
        {
            return lv_audioList.Items.Cast<ListViewItem>().Any(item => item.SubItems[2].Text.Contains("AC3"));
        }

        /// <summary>
        /// Load an arraylist of AudioTrack items into the list.
        /// </summary>
        /// <param name="audioTracks"></param>
        public void LoadTracks(ArrayList audioTracks)
        {
            ClearAudioList();

            if (audioTracks == null)
                return;

            foreach (AudioTrack track in audioTracks)
            {
                ListViewItem newTrack = new ListViewItem(GetNewID().ToString());

                newTrack.SubItems.Add("Automatic");
                newTrack.SubItems.Add(track.Encoder);
                newTrack.SubItems.Add(track.MixDown);
                newTrack.SubItems.Add(track.SampleRate);
                newTrack.SubItems.Add(track.Encoder.Contains("AC3") ? "Auto" : track.Bitrate);
                newTrack.SubItems.Add(track.DRC);
                AddTrackForPreset(newTrack);
            }
        }

        /// <summary>
        /// Set the Track list dropdown from the parsed title captured during the scan
        /// </summary>
        /// <param name="selectedTitle"></param>
        public void SetTrackList(Title selectedTitle)
        {
            drp_audioTrack.Items.Clear();
            drp_audioTrack.Items.Add("Automatic");
            drp_audioTrack.Items.Add("None");
            drp_audioTrack.Items.AddRange(selectedTitle.AudioTracks.ToArray());

            // Handle Native Language and "Dub Foreign language audio" and "Use Foreign language audio and Subtitles" Options
            if (Properties.Settings.Default.NativeLanguage == "Any")
                drp_audioTrack.SelectedIndex = drp_audioTrack.Items.Count >= 3 ? 2 : 0;
            else
            {
                if (Properties.Settings.Default.DubAudio) // "Dub Foreign language audio" 
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
                        foreach (ListViewItem item in lv_audioList.Items)
                            item.SubItems[1].Text = drp_audioTrack.SelectedItem.ToString();
                    else
                    {
                        drp_audioTrack.SelectedIndex = 0;
                        if (drp_audioTrack.SelectedItem != null)
                            foreach (ListViewItem item in lv_audioList.Items)
                                item.SubItems[1].Text = drp_audioTrack.SelectedItem.ToString();
                    }
                }
                else
                    drp_audioTrack.SelectedIndex = drp_audioTrack.Items.Count >= 3 ? 2 : 0;
                        // "Use Foreign language audio and Subtitles"
            }
            drp_audioMix.SelectedIndex = 0;
        }

        // Control and ListView
        private void controlChanged(object sender, EventArgs e)
        {
            Control ctl = (Control) sender;

            switch (ctl.Name)
            {
                case "drp_audioTrack":
                    if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
                        lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[1].Text = drp_audioTrack.Text;
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
                    if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
                        lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[2].Text = drp_audioEncoder.Text;
                    break;
                case "drp_audioMix":
                    SetBitrate();

                    if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
                        lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[3].Text = drp_audioMix.Text;
                    break;
                case "drp_audioSample":
                    if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
                        lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text = drp_audioSample.Text;
                    break;
                case "drp_audioBitrate":
                    // Update an item in the Audio list if required.
                    if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
                        lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text = drp_audioBitrate.Text;
                    break;
                case "tb_drc":
                    double value;
                    if (tb_drc.Value == 0) value = 0;
                    else
                        value = ((tb_drc.Value - 1)/10.0) + 1;

                    lbl_drc.Text = value.ToString();

                    // Update an item in the Audio list if required.
                    if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
                    {
                        lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text = value.ToString();
                        lv_audioList.Select();
                    }
                    break;
            }

            lv_audioList.Select();
        }

        private void lv_audioList_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the Audio List.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                drp_audioTrack.SelectedItem = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[1].Text;
                drp_audioEncoder.SelectedItem = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[2].Text;
                drp_audioMix.SelectedItem = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[3].Text;
                drp_audioSample.SelectedItem = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text;
                drp_audioBitrate.SelectedItem = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text;
                double drcValue;
                int drcCalculated;
                double.TryParse(lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text, out drcValue);
                if (drcValue != 0)
                    drcValue = ((drcValue*10) + 1) - 10;
                int.TryParse(drcValue.ToString(), out drcCalculated);
                tb_drc.Value = drcCalculated;
                lbl_drc.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text;

                AudioTrackGroup.Text = "Selected Track: " +
                                       lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[0].Text;
            }
            else
                AudioTrackGroup.Text = "Selected Track: None (Click \"Add Track\" to add)";
        }

        // Track Controls
        private void btn_addAudioTrack_Click(object sender, EventArgs e)
        {
            double value = 0;
            if (tb_drc.Value != 0)
                value = ((tb_drc.Value - 1)/10.0) + 1;

            // Create a new row for the Audio list based on the currently selected items in the dropdown.
            ListViewItem newTrack = new ListViewItem(GetNewID().ToString());
            newTrack.SubItems.Add(drp_audioTrack.Text);
            newTrack.SubItems.Add(drp_audioEncoder.Text);
            newTrack.SubItems.Add(drp_audioMix.Text);
            newTrack.SubItems.Add(drp_audioSample.Text);
            newTrack.SubItems.Add(drp_audioBitrate.Text);
            newTrack.SubItems.Add(value.ToString());
            lv_audioList.Items.Add(newTrack);

            // The Audio List has changed to raise the event.
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());

            // Select the newly added track and select the control       
            lv_audioList.Items[lv_audioList.Items.Count - 1].Selected = true;
            lv_audioList.Select();
        }

        private void btn_RemoveAudioTrack_Click(object sender, EventArgs e)
        {
            RemoveTrack();
        }

        // Audio List Menu
        private void audioList_moveup_Click(object sender, EventArgs e)
        {
            MoveTrack(true);
        }

        private void audioList_movedown_Click(object sender, EventArgs e)
        {
            MoveTrack(false);
        }

        private void audioList_remove_Click(object sender, EventArgs e)
        {
            RemoveTrack();
        }

        // Public Functions
        private void AddTrackForPreset(ListViewItem item)
        {
            lv_audioList.Items.Add(item);
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
        }

        private void ClearAudioList()
        {
            lv_audioList.Items.Clear();
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
        }

        private int GetNewID()
        {
            return lv_audioList.Items.Count + 1;
        }

        private void RemoveTrack()
        {
            // Remove the Item and reselect the control if the following conditions are met.
            if (lv_audioList.SelectedItems.Count != 0)
            {
                // The Audio List is about to change so raise the event.
                if (this.AudioListChanged != null)
                    this.AudioListChanged(this, new EventArgs());

                // Record the current selected index.
                int currentPosition = lv_audioList.SelectedIndices[0];

                lv_audioList.Items.RemoveAt(lv_audioList.SelectedIndices[0]);

                // Now reslect the correct item and give focus to the audio list.
                if (lv_audioList.Items.Count != 0)
                {
                    if (currentPosition <= (lv_audioList.Items.Count - 1))
                        lv_audioList.Items[currentPosition].Selected = true;
                    else if (currentPosition > (lv_audioList.Items.Count - 1))
                        lv_audioList.Items[lv_audioList.Items.Count - 1].Selected = true;

                    lv_audioList.Select();
                }
                // Regenerate the ID numers
                ReGenerateListIDs();
            }
        }

        private void MoveTrack(bool up)
        {
            if (lv_audioList.SelectedIndices.Count == 0) return;

            ListViewItem item = lv_audioList.SelectedItems[0];
            int index = item.Index;

            if (up) index--;
            else index++;

            if (index < lv_audioList.Items.Count || (lv_audioList.Items.Count > index && index >= 0))
            {
                lv_audioList.Items.Remove(item);
                lv_audioList.Items.Insert(index, item);
                item.Selected = true;
                lv_audioList.Focus();
            }
        }

        private void ReGenerateListIDs()
        {
            int i = 1;
            foreach (ListViewItem item in lv_audioList.Items)
            {
                item.SubItems[0].Text = i.ToString();
                i++;
            }
        }

        private void SetBitrate()
        {
            int max = 0;
            drp_audioBitrate.Items.Remove("Auto");
            drp_audioBitrate.Items.Remove("192");
            drp_audioBitrate.Items.Remove("224");
            drp_audioBitrate.Items.Remove("256");
            drp_audioBitrate.Items.Remove("320");

            switch (drp_audioEncoder.Text)
            {
                case "AAC (faac)":
                    max = drp_audioMix.Text.Contains("6 Channel") ? 384 : 160;
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
                if (max == 384)
                    drp_audioBitrate.Items.Add("384");
                else
                    drp_audioBitrate.Items.Remove("384");
            }
            else
            {
                drp_audioBitrate.Items.Remove("192");
                drp_audioBitrate.Items.Remove("224");
                drp_audioBitrate.Items.Remove("256");
                drp_audioBitrate.Items.Remove("320");
                drp_audioBitrate.Items.Remove("384");
            }

            if (drp_audioBitrate.SelectedItem == null)
                drp_audioBitrate.SelectedIndex = drp_audioBitrate.Items.Count - 1;
        }

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
                    drp_audioMix.Items.Remove("Mono");
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