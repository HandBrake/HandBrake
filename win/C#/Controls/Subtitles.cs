using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Handbrake.Functions;
using System.IO;

namespace Handbrake.Controls
{
    public partial class Subtitles : UserControl
    {

        IDictionary<string, string> LangMap = new Dictionary<string, string>();

        public Subtitles()
        {
            InitializeComponent();

            LangMap = Main.mapLanguages();
            foreach (string key in LangMap.Keys)
                srt_lang.Items.Add(key);

            srt_charcode.SelectedIndex = 28;
            srt_lang.SelectedIndex = 40;
        }

        private int fileContainer;
        public void setContainer(int value)
        {
            fileContainer = value;
            Boolean trigger = false;
            if (fileContainer != 1)
                foreach (ListViewItem item in lv_subList.Items)
                {
                    if (item.SubItems[1].Text.Contains("Bitmap"))
                    {
                        if (trigger)
                            lv_subList.Items.Remove(item);
                        trigger = true;
                    }
                }
        }

        // Controls
        private void btn_addSubTrack_Click(object sender, EventArgs e)
        {
            string forcedVal = "No";
            string burnedVal = "No";
            string defaultSub = "No";

            if (check_forced.Checked)
                forcedVal = "Yes";

            if (check_burned.Checked)
            {
                if (!drp_subtitleTracks.Text.Contains("Text"))
                {
                    burnedVal = "Yes";
                    setNoBurned();
                }
            }

            if (check_default.Checked)
            {
                defaultSub = "Yes";
                setNoDefault();
            }

            Boolean addTrack = true;
            if (fileContainer == 0)
            {
                burnedVal = "Yes";  // MP4 must have bitmap subs burned in.

                // Make sure we only have 1 bitmap track.
                if (drp_subtitleTracks.SelectedItem != null)
                {
                    if (drp_subtitleTracks.SelectedItem.ToString().Contains("Bitmap"))
                        foreach (ListViewItem item in lv_subList.Items)
                        {
                            if (item.SubItems[1].Text.Contains("Bitmap"))
                            {
                                MessageBox.Show(this,
                                                "More than one vobsub is not supported in mp4... Your first vobsub track will now be used.",
                                                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                                addTrack = false;
                            }
                        }
                }
                else
                    addTrack = false;

            }

            // Add the track if allowed.
            if (addTrack)
            {
                ListViewItem newTrack = new ListViewItem(getNewID().ToString());

                newTrack.SubItems.Add(drp_subtitleTracks.SelectedItem.ToString());
                newTrack.SubItems.Add(forcedVal);
                newTrack.SubItems.Add(burnedVal);
                newTrack.SubItems.Add(defaultSub);

                lv_subList.Items.Add(newTrack);
            }
        }
        private void btn_srtAdd_Click(object sender, EventArgs e)
        {
            ListViewItem newTrack = new ListViewItem(getNewID().ToString());

            newTrack.SubItems.Add(srt_lang.SelectedItem + ", (" + srt_charcode.SelectedItem + ")");
            newTrack.SubItems.Add("No");
            newTrack.SubItems.Add("No");
            newTrack.SubItems.Add("No");
            if (openFileDialog.FileName != null)
                newTrack.SubItems.Add(openFileDialog.FileName);
            else
                newTrack.SubItems.Add("None");

            newTrack.SubItems.Add(srt_offset.Value.ToString());

            lv_subList.Items.Add(newTrack);
        }
        private void btn_RemoveSubTrack_Click(object sender, EventArgs e)
        {
            removeTrack();
        }
        private void lb_subList_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the List.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                if (lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems.Count != 5)  // We have an SRT
                {
                    string[] trackData = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text.Split(',');
                    string charCode = trackData[1].Replace("(", "").Replace(")", "");
                    srt_lang.SelectedItem = trackData[0];
                    srt_charcode.SelectedItem = charCode.Trim();

                    int offsetVal;
                    int.TryParse(lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[6].Text, out offsetVal);
                    srt_offset.Value = offsetVal;

                    SRTGroup.Text = "Selected Track: " + lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text;
                    SubTitlesGroup.Text = "Selected Track: None";
                }
                else  // We have Bitmap/CC
                {
                    // Setup the controls
                    int c = 0;
                    foreach (var item in drp_subtitleTracks.Items)
                    {
                        if (item.ToString() == lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text)
                            drp_subtitleTracks.SelectedIndex = c;
                        c++;
                    }
                    drp_subtitleTracks.SelectedItem = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1];

                    check_forced.CheckState = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[2].Text == "Yes" ? CheckState.Checked : CheckState.Unchecked;
                    check_burned.CheckState = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[3].Text == "Yes" ? CheckState.Checked : CheckState.Unchecked;  
                    check_default.CheckState = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[4].Text == "Yes" ? CheckState.Checked : CheckState.Unchecked;

                    SubTitlesGroup.Text = "Selected Track: " + lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text;
                    SRTGroup.Text = "Selected Track: None";
                }
            }
            else
            {
                SubTitlesGroup.Text = "Selected Track: None (Click \"Add Track\" to add)";
                SRTGroup.Text = "Selected Track: None (Click \"Add External SRT\" to add) ";
            }
        }

        // Bitmap / CC Controls
        private void drp_subtitleTracks_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text = drp_subtitleTracks.SelectedItem.ToString();
                lv_subList.Select();
            }
        }
        private void check_forced_CheckedChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[2].Text = check_forced.Checked ? "Yes" : "No";
                lv_subList.Select();
            }
        }
        private void check_burned_CheckedChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                if (check_burned.Checked) // Make sure we only have 1 burned track
                    setNoBurned();

                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[3].Text = check_burned.Checked ? "Yes" : "No";
                lv_subList.Select();
            }
        }
        private void check_default_CheckedChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                if (check_default.Checked) // Make sure we only have 1 default track
                    setNoDefault();

                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[4].Text = check_default.Checked ? "Yes" : "No";
                lv_subList.Select();
            }
        }

        // SRT Controls
        private void srt_offset_ValueChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[6].Text = srt_offset.Value.ToString();
                lv_subList.Select();
            }
        }
        private void srt_charcode_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                string[] trackData = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text.Split(',');

                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text = trackData[0].Trim() + ", (" + srt_charcode.SelectedItem + ")";
                lv_subList.Select();
            }
        }
        private void srt_lang_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                string[] trackData = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text.Split(',');
                string charCode = trackData[1].Replace("(", "").Replace(")", "").Trim();

                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text = srt_lang.SelectedItem + ", (" + charCode + ")";
                lv_subList.Select();
            }
        }
        private void srt_browse_Click(object sender, EventArgs e)
        {
            openFileDialog.ShowDialog();
        }

        // Right Click Menu
        private void mnu_moveup_Click(object sender, EventArgs e)
        {
            if (lv_subList.SelectedIndices.Count != 0)
            {
                ListViewItem item = lv_subList.SelectedItems[0];
                int index = item.Index;
                index--;

                if (lv_subList.Items.Count > index && index >= 0)
                {
                    lv_subList.Items.Remove(item);
                    lv_subList.Items.Insert(index, item);
                    item.Selected = true;
                    lv_subList.Focus();
                }
            }
        }
        private void mnu_movedown_Click(object sender, EventArgs e)
        {
            if (lv_subList.SelectedIndices.Count != 0)
            {
                ListViewItem item = lv_subList.SelectedItems[0];
                int index = item.Index;
                index++;

                if (index < lv_subList.Items.Count)
                {
                    lv_subList.Items.Remove(item);
                    lv_subList.Items.Insert(index, item);
                    item.Selected = true;
                    lv_subList.Focus();
                }
            }
        }
        private void mnu_remove_Click(object sender, EventArgs e)
        {
            removeTrack();
        }

        // Functions
        private void setNoDefault()
        {
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (item.SubItems[4].Text == "Yes")
                    item.SubItems[4].Text = "No";
            }
        }
        private void setNoBurned()
        {
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (item.SubItems[3].Text == "Yes")
                    item.SubItems[3].Text = "No";
            }
        }
        private void removeTrack()
        {
            // Remove the Item and reselect the control if the following conditions are met.
            if (lv_subList.SelectedItems.Count != 0)
            {
                // Record the current selected index.
                int currentPosition = lv_subList.SelectedIndices[0];

                lv_subList.Items.RemoveAt(lv_subList.SelectedIndices[0]);

                // Now reslect the correct item and give focus to the list.
                if (lv_subList.Items.Count != 0)
                {
                    if (currentPosition <= (lv_subList.Items.Count - 1))
                        lv_subList.Items[currentPosition].Selected = true;
                    else if (currentPosition > (lv_subList.Items.Count - 1))
                        lv_subList.Items[lv_subList.Items.Count - 1].Selected = true;

                    lv_subList.Select();
                }

                // Regenerate the ID numers
                reGenerateListIDs();
            }
        }

        // Helper Functions
        private int getNewID()
        {
            return lv_subList.Items.Count + 1;
        }
        private void reGenerateListIDs()
        {
            int i = 1;
            foreach (ListViewItem item in lv_subList.Items)
            {
                item.SubItems[0].Text = i.ToString();
                i++;
            }
        }

    }
}
