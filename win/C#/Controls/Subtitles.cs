using System;
using System.Windows.Forms;

namespace Handbrake.Controls
{
    public partial class Subtitles : UserControl
    {

        // TODO
        // - Right click menu for adding/removal of tracks.
        // - Multi-select for removal.

        public Subtitles()
        {
            InitializeComponent();
        }
        public int setContainer { get; set; }

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
            if (setContainer == 0 || setContainer == 1)
            {
                burnedVal = "Yes";  // MP4 must have bitmap subs burned in.

                // Make sure we only have 1 bitmap track.
                if (drp_subtitleTracks.SelectedItem.ToString().Contains("Bitmap"))
                    foreach (ListViewItem item in lv_subList.Items)
                    {
                        if (item.SubItems[1].Text.Contains("Bitmap"))
                        {
                            MessageBox.Show(this,
                                            "MP4 files can only have 1 bitmap track. If you wish to have multiple bitmap tracks you should consider using MKV if suitable.",
                                            "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                            addTrack = false;
                        }
                    }
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
        private void btn_RemoveSubTrack_Click(object sender, EventArgs e)
        {
            removeTrack();
        }
        private void lb_subList_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the List.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                // Reset the checkboxes
                check_forced.CheckState = CheckState.Unchecked;
                check_burned.CheckState = CheckState.Unchecked;
                check_default.CheckState = CheckState.Unchecked;

                // Setup the controls
                int c = 0;
                foreach (var item in drp_subtitleTracks.Items)
                {
                    if (item.ToString() == lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text)
                        drp_subtitleTracks.SelectedIndex = c;
                    c++;
                }
                drp_subtitleTracks.SelectedItem = lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1];

                if (lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[2].Text == "Yes")
                    check_forced.CheckState = CheckState.Checked;

                if (lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[3].Text == "Yes")
                    check_burned.CheckState = CheckState.Checked;

                if (lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[4].Text == "Yes")
                    check_default.CheckState = CheckState.Checked;

                AudioTrackGroup.Text = "Selected Track: " + lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text;
            }
            else
                AudioTrackGroup.Text = "Selected Track: None (Click \"Add Track\" to add)";
        }

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
