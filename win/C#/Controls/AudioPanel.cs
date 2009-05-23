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
        public AudioPanel()
        {
            InitializeComponent();
        }
        
        // Audio Track Options
        private void drp_track1Audio_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[1].Text = drp_track1Audio.Text;
                lv_audioList.Select();
            }

        }
        private void drp_audenc_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drp_audenc_1.Text.Contains("AC3") || drp_audenc_1.Text.Contains("DTS"))
            {
                drp_audmix_1.Enabled = false;
                drp_audbit_1.Enabled = false;
                drp_audsr_1.Enabled = false;

                drp_audmix_1.SelectedIndex = 0;
                drp_audbit_1.SelectedIndex = 0;
                drp_audsr_1.SelectedIndex = 0;
            }
            else
            {
                drp_audmix_1.Enabled = true;
                drp_audbit_1.Enabled = true;
                drp_audsr_1.Enabled = true;

                drp_audmix_1.Text = "Automatic";
                drp_audbit_1.Text = "160";
                drp_audsr_1.Text = "Auto";
            }

            if (drp_audenc_1.Text.Contains("AAC"))
            {
                setMixDownAllOptions(drp_audmix_1);
                setBitrateSelections160(drp_audbit_1);
            }
            else
            {
                setMixDownNotAAC(drp_audmix_1);
                setBitrateSelections320(drp_audbit_1);
            }

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[2].Text = drp_audenc_1.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audmix_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((drp_audenc_1.Text.Contains("AAC")) && (drp_audmix_1.Text == "6 Channel Discrete"))
                setBitrateSelections384(drp_audbit_1);
            else if ((drp_audenc_1.Text.Contains("AAC")) && (drp_audmix_1.Text != "6 Channel Discrete"))
                setBitrateSelections160(drp_audbit_1); drp_audbit_1.Text = "160";

            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[3].Text = drp_audmix_1.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audsr_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text = drp_audsr_1.Text;
                lv_audioList.Select();
            }
        }
        private void drp_audbit_1_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the Audio list if required.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                if (drp_audenc_1.Text.Contains("AC3"))
                    drp_audbit_1.Text = "Auto";
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text = drp_audbit_1.Text;
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
                lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text = lbl_drc.Text;
                lv_audioList.Select();
            }
        }

        // Track Controls
        private void btn_addAudioTrack_Click(object sender, EventArgs e)
        {
            // Create a new row for the Audio list based on the currently selected items in the dropdown.
            ListViewItem newTrack = new ListViewItem(getNewID().ToString());
            newTrack.SubItems.Add(drp_track1Audio.Text);
            newTrack.SubItems.Add(drp_audenc_1.Text);
            newTrack.SubItems.Add(drp_audmix_1.Text);
            newTrack.SubItems.Add(drp_audsr_1.Text);
            newTrack.SubItems.Add(drp_audbit_1.Text);
            newTrack.SubItems.Add(lbl_drc.Text);
            lv_audioList.Items.Add(newTrack);

            // Select the newly added track and select the control       
            lv_audioList.Items[lv_audioList.Items.Count - 1].Selected = true;
            lv_audioList.Select();
        }
        private void btn_RemoveAudioTrack_Click(object sender, EventArgs e)
        {
            // Remove the Item and reselect the control if the following conditions are met.
            if (lv_audioList.SelectedItems.Count != 0)
            {
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
            }
        }

        // Public Functions
        public void setTrackList(Parsing.Title selectedTitle)
        {
            drp_track1Audio.Items.Clear();
            drp_track1Audio.Items.Add("Automatic");
            drp_track1Audio.Items.Add("None");
            drp_track1Audio.Items.AddRange(selectedTitle.AudioTracks.ToArray());
            drp_track1Audio.SelectedIndex = 0;
        }
        public ListView getAudioPanel()
        {
            return lv_audioList;
        }
        public void setAudioByContainer(String path)
        {
            if ((path.Contains("MP4")) || (path.Contains("M4V")))
            {
                string oldval = drp_audenc_1.Text;
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("AAC (faac)");
                drp_audenc_1.Items.Add("AC3 Passthru");
                if ((oldval != "AAC (faac)") && (oldval != "AC3 Passthru"))
                    drp_audenc_1.SelectedIndex = 0;

            }
            else if (path.Contains("MKV"))
            {
                drp_audenc_1.Items.Clear();
                drp_audenc_1.Items.Add("AAC (faac)");
                drp_audenc_1.Items.Add("MP3 (lame)");
                drp_audenc_1.Items.Add("AC3 Passthru");
                drp_audenc_1.Items.Add("DTS Passthru");
                drp_audenc_1.Items.Add("Vorbis (vorbis)");

                if (drp_audenc_1.Text == string.Empty)
                    drp_audenc_1.SelectedIndex = 0;
            }

            // Make sure the table is updated with new audio codecs
            foreach (ListViewItem row in lv_audioList.Items)
            {
                if (!drp_audenc_1.Items.Contains(row.SubItems[2].Text))
                    row.SubItems[2].Text = drp_audenc_1.Items[0].ToString();
            }
        }
        public void addTrackForPreset(ListViewItem item)
        {
            lv_audioList.Items.Add(item);
        }
        public void clearAudioList()
        {
            lv_audioList.Items.Clear();
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
        private static void setBitrateSelections384(ComboBox dropDown)
        {
            dropDown.Items.Clear();
            dropDown.Items.Add("32");
            dropDown.Items.Add("40");
            dropDown.Items.Add("48");
            dropDown.Items.Add("56");
            dropDown.Items.Add("64");
            dropDown.Items.Add("80");
            dropDown.Items.Add("86");
            dropDown.Items.Add("112");
            dropDown.Items.Add("128");
            dropDown.Items.Add("160");
            dropDown.Items.Add("192");
            dropDown.Items.Add("224");
            dropDown.Items.Add("256");
            dropDown.Items.Add("320");
            dropDown.Items.Add("384");
        }
        private static void setBitrateSelections320(ComboBox dropDown)
        {
            dropDown.Items.Clear();
            dropDown.Items.Add("32");
            dropDown.Items.Add("40");
            dropDown.Items.Add("48");
            dropDown.Items.Add("56");
            dropDown.Items.Add("64");
            dropDown.Items.Add("80");
            dropDown.Items.Add("86");
            dropDown.Items.Add("112");
            dropDown.Items.Add("128");
            dropDown.Items.Add("160");
            dropDown.Items.Add("192");
            dropDown.Items.Add("224");
            dropDown.Items.Add("256");
            dropDown.Items.Add("320");
        }
        private static void setBitrateSelections160(ComboBox dropDown)
        {
            dropDown.Items.Clear();
            dropDown.Items.Add("32");
            dropDown.Items.Add("40");
            dropDown.Items.Add("48");
            dropDown.Items.Add("56");
            dropDown.Items.Add("64");
            dropDown.Items.Add("80");
            dropDown.Items.Add("86");
            dropDown.Items.Add("112");
            dropDown.Items.Add("128");
            dropDown.Items.Add("160");
        }
        private static void setMixDownAllOptions(ComboBox dropdown)
        {
            dropdown.Items.Clear();
            dropdown.Items.Add("Automatic");
            dropdown.Items.Add("Mono");
            dropdown.Items.Add("Stereo");
            dropdown.Items.Add("Dolby Surround");
            dropdown.Items.Add("Dolby Pro Logic II");
            dropdown.Items.Add("6 Channel Discrete");
        }
        private static void setMixDownNotAAC(ComboBox dropdown)
        {
            dropdown.Items.Clear();
            dropdown.Items.Add("Automatic");
            dropdown.Items.Add("Stereo");
            dropdown.Items.Add("Dolby Surround");
            dropdown.Items.Add("Dolby Pro Logic II");
        }
        private void lv_audioList_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the Audio List.
            if (lv_audioList.Items.Count != 0 && lv_audioList.SelectedIndices.Count != 0)
            {
                drp_track1Audio.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[1].Text;
                drp_audenc_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[2].Text;
                drp_audmix_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[3].Text;
                drp_audsr_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[4].Text;
                drp_audbit_1.Text = lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[5].Text;
                double drcValue; int drcCalculated;
                double.TryParse(lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[6].Text, out drcValue);
                if (drcValue == 0) drcCalculated = 0;
                else
                    drcValue = ((drcValue * 10) + 1) - 10;
                int.TryParse(drcValue.ToString(), out drcCalculated);
                tb_drc.Value = drcCalculated;

                AudioTrackGroup.Text = "Selected Track: " + lv_audioList.Items[lv_audioList.SelectedIndices[0]].SubItems[0].Text;
            }
            else
                AudioTrackGroup.Text = "Selected Track: None (Click \"Add Track\" to add)";
        }

    }
}
