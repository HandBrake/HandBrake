/*  AudioPanel.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;

namespace Handbrake.Controls
{
    public partial class AudioPanel : UserControl
    {
        public event EventHandler AudioListChanged;

        public AudioPanel()
        {
            InitializeComponent();
            drp_audioMix.SelectedIndex = 0;
        }
        
        // Audio Track Options
        private void drp_audioTrack_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[1].Text = drp_audioTrack.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audenc_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Setup the widgets with the correct avail options
            if (drp_audioEncoder.Text.Contains("AAC"))
            {
                setMixDown(true);
                setBitrate(160);
            }
            else
            {
                setMixDown(false);
                setBitrate(320);
            }

            // Configure the widgets with values
            if (drp_audioEncoder.Text.Contains("AC3") || drp_audioEncoder.Text.Contains("DTS"))
            {
                drp_audioMix.Enabled = false;
                drp_audioBitrate.Enabled = false;
                drp_audioSample.Enabled = false;

                drp_audioMix.SelectedIndex = 0;
                drp_audioBitrate.SelectedIndex = 0;
                drp_audioSample.SelectedIndex = 0;
            }
            else
            {
                drp_audioMix.Enabled = true;
                drp_audioBitrate.Enabled = true;
                drp_audioSample.Enabled = true;

                drp_audioMix.SelectedIndex = 0;
                drp_audioBitrate.SelectedIndex = 9;
                drp_audioSample.SelectedIndex = 0;
            }

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[2].Text = drp_audioEncoder.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audmix_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((drp_audioEncoder.Text.Contains("AAC")) && (drp_audioMix.Text == "6 Channel Discrete"))
                setBitrate(384);
            else if ((drp_audioEncoder.Text.Contains("AAC")) && (drp_audioMix.Text != "6 Channel Discrete"))
                setBitrate(160);

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[3].Text = drp_audioMix.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audsr_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text = drp_audioSample.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audbit_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                if (drp_audioEncoder.Text.Contains("AC3"))
                    drp_audioBitrate.SelectedItem = "Auto";
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text = drp_audioBitrate.Text;
                lv_audioList.Select();
            }
        }
        private void tb_drc_Scroll(object sender, EventArgs e)
        {
            double value;
            if (tb_drc.Value == 0) value = 0;
            else
                value = ((tb_drc.Value - 1) / 10.0) + 1;

            lbl_drc.Text = value.ToString();

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text = value.ToString();
                lv_audioList.Select();
            }
        }

        // Track Controls
        private void btn_addAudioTrack_Click(object sender, EventArgs e)
        {
            double value = 0;
            if (tb_drc.Value != 0)
                value = ((tb_drc.Value - 1) / 10.0) + 1;

            // Create a new row for the Audio list based on the currently selected items in the dropdown.
            ListViewItem newTrack = new ListViewItem(getNewID().ToString());
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
            removeAudioTrack();
        }

        // Audio List Menu
        private void audioList_moveup_Click(object sender, EventArgs e)
        {
            if (lv_audioList.SelectedIndices.Count != 0)
            {
                ListViewItem item = lv_audioList.SelectedItems[0];
                int index = item.Index;
                index--;

                if (lv_audioList.Items.Count > index && index >= 0)
                {
                    lv_audioList.Items.Remove(item);
                    lv_audioList.Items.Insert(index, item);
                    item.Selected = true;
                    lv_audioList.Focus();
                }
            }
        }
        private void audioList_movedown_Click(object sender, EventArgs e)
        {
            if (lv_audioList.SelectedIndices.Count != 0)
            {
                ListViewItem item = lv_audioList.SelectedItems[0];
                int index = item.Index;
                index++;

                if (index < lv_audioList.Items.Count)
                {
                    lv_audioList.Items.Remove(item);
                    lv_audioList.Items.Insert(index, item);
                    item.Selected = true;
                    lv_audioList.Focus();
                }
            }
        }
        private void audioList_remove_Click(object sender, EventArgs e)
        {
            removeAudioTrack();
        }
        private void removeAudioTrack()
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
                reGenerateListIDs();
            }
        }

        // Public Functions
        public void setTrackList(Parsing.Title selectedTitle)
        {
            drp_audioTrack.Items.Clear();
            drp_audioTrack.Items.Add("Automatic");
            drp_audioTrack.Items.Add("None");
            drp_audioTrack.Items.AddRange(selectedTitle.AudioTracks.ToArray());
            drp_audioTrack.SelectedIndex = 0;
            drp_audioMix.SelectedIndex = 0;
        }
        public ListView getAudioPanel()
        {
            return lv_audioList;
        }
        public void setAudioByContainer(String path)
        {
            if ((path.Contains("MP4")) || (path.Contains("M4V")))
            {
                string oldval = drp_audioEncoder.Text;
                drp_audioEncoder.Items.Clear();
                drp_audioEncoder.Items.Add("AAC (faac)");
                drp_audioEncoder.Items.Add("AC3 Passthru");
                if ((oldval != "AAC (faac)") && (oldval != "AC3 Passthru"))
                    drp_audioEncoder.SelectedIndex = 0;

            }
            else if (path.Contains("MKV"))
            {
                drp_audioEncoder.Items.Clear();
                drp_audioEncoder.Items.Add("AAC (faac)");
                drp_audioEncoder.Items.Add("MP3 (lame)");
                drp_audioEncoder.Items.Add("AC3 Passthru");
                drp_audioEncoder.Items.Add("DTS Passthru");
                drp_audioEncoder.Items.Add("Vorbis (vorbis)");

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
        public void addTrackForPreset(ListViewItem item)
        {
            lv_audioList.Items.Add(item);
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
        }
        public void clearAudioList()
        {
            lv_audioList.Items.Clear();
            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
        }
        public int getNewID()
        {
            return lv_audioList.Items.Count + 1;
        }

        // Helper Functions 
        private void reGenerateListIDs()
        {
            int i = 1;
            foreach (ListViewItem item in lv_audioList.Items)
            {
                item.SubItems[0].Text = i.ToString();
                i++;
            }
        }
        private void setBitrate(int max)
        {
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
        private void setMixDown(Boolean aac)
        {
            drp_audioMix.Items.Clear();
            drp_audioMix.Items.Add("Automatic");
            if (aac)
                drp_audioMix.Items.Add("Mono");
            drp_audioMix.Items.Add("Stereo");
            drp_audioMix.Items.Add("Dolby Surround");
            drp_audioMix.Items.Add("Dolby Pro Logic II");
            if (aac)
                drp_audioMix.Items.Add("6 Channel Discrete");
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
                double drcValue; int drcCalculated;
                double.TryParse(lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text, out drcValue);
                if (drcValue == 0) drcCalculated = 0;
                else
                    drcValue = ((drcValue * 10) + 1) - 10;
                int.TryParse(drcValue.ToString(), out drcCalculated);
                tb_drc.Value = drcCalculated;
                lbl_drc.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text;

                AudioTrackGroup.Text = "Selected Track: " + lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[0].Text;
            }
            else
                AudioTrackGroup.Text = "Selected Track: None (Click \"Add Track\" to add)";
        }

    }
}