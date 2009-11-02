using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using Handbrake.Functions;

namespace Handbrake.Controls
{
    public partial class Subtitles : UserControl
    {
        readonly IDictionary<string, string> LangMap = new Dictionary<string, string>();
        List<SubtitleInfo> SubList = new List<SubtitleInfo>();
        private int _fileContainer;

        public Subtitles()
        {
            InitializeComponent();

            LangMap = Main.mapLanguages();
            foreach (string key in LangMap.Keys)
                srt_lang.Items.Add(key);

            srt_charcode.SelectedIndex = 28;
            srt_lang.SelectedIndex = 40;
        }

        // Primary Controls
        private void btn_addSubTrack_Click(object sender, EventArgs e)
        {
            // Logic
            string forcedVal = check_forced.CheckState == CheckState.Checked ? "Yes" : "No";
            string defaultSub = check_default.CheckState == CheckState.Checked ? "Yes" : "No";
            string burnedVal = check_burned.CheckState == CheckState.Checked && (!drp_subtitleTracks.Text.Contains("Text")) ? "Yes" : "No";
            string srtCode = "-", srtLangVal = "-", srtPath = "-", srtFile = "-";
            int srtOffsetMs = 0;

            if (drp_subtitleTracks.SelectedItem.ToString().Contains(".srt"))
            {
                burnedVal = "No";
                forcedVal = "No";
                srtPath = openFileDialog.FileName;
                srtFile = Path.GetFileName(openFileDialog.FileName);
                srtLangVal = srt_lang.SelectedItem.ToString();
                srtCode = srt_charcode.SelectedItem.ToString();
                srtOffsetMs = (int)srt_offset.Value;
            }

            if (_fileContainer == 0) // MP4 and M4V file extension
            {
                // Make sure we only have 1 bitmap track.
                if (drp_subtitleTracks.SelectedItem != null && drp_subtitleTracks.SelectedItem.ToString().Contains("Bitmap"))
                {
                    foreach (ListViewItem item in lv_subList.Items)
                    {
                        if (item.SubItems[0].Text.Contains("Bitmap"))
                        {
                            MessageBox.Show(this, "More than one vobsub is not supported in mp4... Your first vobsub track will now be used.",
                                            "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                            return;
                        }
                    }
                }
            }

            if (defaultSub == "Yes") SetNoDefault();
            if (burnedVal == "Yes") SetNoBurned();

            if (_fileContainer == 0 && !drp_subtitleTracks.SelectedItem.ToString().Contains(".srt"))
                burnedVal = "Yes";  // MP4 must have bitmap subs burned in.

            string trackName = (drp_subtitleTracks.SelectedItem.ToString().Contains(".srt"))
                               ? srtLangVal + " (" + srtFile + ")"
                               : drp_subtitleTracks.SelectedItem.ToString();


            SubtitleInfo track = new SubtitleInfo
                                     {
                                         Track = trackName,
                                         Forced = forcedVal,
                                         Burned = burnedVal,
                                         Default = defaultSub,
                                         SrtLang = srtLangVal,
                                         SrtCharCode = srtCode,
                                         SrtOffset = srtOffsetMs,
                                         SrtPath = srtPath,
                                         SrtFileName = srtFile
                                     };

            lv_subList.Items.Add(track.ListView);
            SubList.Add(track);

        }
        private void btn_srtAdd_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() != DialogResult.OK)
                return;
            drp_subtitleTracks.Items.Add(Path.GetFileName(openFileDialog.FileName));
            drp_subtitleTracks.SelectedItem = Path.GetFileName(openFileDialog.FileName);
        }
        private void btn_RemoveSubTrack_Click(object sender, EventArgs e)
        {
            RemoveTrack();
        }

        // Secondary Controls
        private void lb_subList_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the List.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                SubtitleInfo track = SubList[lv_subList.SelectedIndices[0]];

                int c = 0;
                if (lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text.ToLower().Contains(".srt"))  // We have an SRT
                {
                    foreach (var item in drp_subtitleTracks.Items)
                    {
                        if (item.ToString() == track.SrtFileName)
                            drp_subtitleTracks.SelectedIndex = c;
                        c++;
                    }
                    srt_lang.SelectedItem = track.SrtLang;
                    srt_offset.Value = track.SrtOffset;
                    srt_charcode.SelectedItem = track.SrtCharCode;
                    check_default.CheckState = track.Default == "Yes" ? CheckState.Checked : CheckState.Unchecked;
                    SubGroupBox.Text = "Selected Track: " + track.Track;
                }
                else  // We have Bitmap/CC
                {
                    foreach (var item in drp_subtitleTracks.Items)
                    {
                        if (item.ToString() == track.Track)
                            drp_subtitleTracks.SelectedIndex = c;
                        c++;
                    }
                    check_forced.CheckState = track.Forced == "Yes" ? CheckState.Checked : CheckState.Unchecked;
                    check_burned.CheckState = track.Burned == "Yes" ? CheckState.Checked : CheckState.Unchecked;
                    check_default.CheckState = track.Default == "Yes" ? CheckState.Checked : CheckState.Unchecked;
                    SubGroupBox.Text = "Selected Track: " + lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text;
                }
            }
            else
                SubGroupBox.Text = "Selected Track: None (Click \"Add\" to add another track to the list)";
        }

        // Bitmap / CC / SRT Controls
        private void drp_subtitleTracks_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (drp_subtitleTracks.SelectedItem.ToString().Contains(".srt"))
            {
                check_forced.Enabled = false;
                check_burned.Enabled = false;
                srt_lang.Enabled = true;
                srt_charcode.Enabled = true;
                srt_offset.Enabled = true;
            }
            else
            {
                check_forced.Enabled = true;
                check_burned.Enabled = true;
                srt_lang.Enabled = false;
                srt_charcode.Enabled = false;
                srt_offset.Enabled = false;
            }
            // Update an item in the  list if required.
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text = drp_subtitleTracks.SelectedItem.ToString();
            lv_subList.Select();

            SubList[lv_subList.SelectedIndices[0]].Track = drp_subtitleTracks.SelectedItem.ToString(); // Update SubList List<SubtitleInfo>
        }
        private void check_forced_CheckedChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text = check_forced.Checked ? "Yes" : "No";
            lv_subList.Select();

            SubList[lv_subList.SelectedIndices[0]].Forced = check_forced.Checked ? "Yes" : "No"; // Update SubList List<SubtitleInfo> 
        }
        private void check_burned_CheckedChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            if (check_burned.Checked) // Make sure we only have 1 burned track
                SetNoBurned();

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[2].Text = check_burned.Checked ? "Yes" : "No";
            lv_subList.Select();

            SubList[lv_subList.SelectedIndices[0]].Burned = check_burned.Checked ? "Yes" : "No"; // Update SubList List<SubtitleInfo> 
        }
        private void check_default_CheckedChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            if (check_default.Checked) // Make sure we only have 1 default track
                SetNoDefault();

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[3].Text = check_default.Checked ? "Yes" : "No";
            lv_subList.Select();

            SubList[lv_subList.SelectedIndices[0]].Default = check_default.Checked ? "Yes" : "No"; // Update SubList List<SubtitleInfo>
        }
        private void srt_offset_ValueChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0)
                return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[6].Text = srt_offset.Value.ToString();
            lv_subList.Select();

            SubList[lv_subList.SelectedIndices[0]].SrtOffset = (int)srt_offset.Value; // Update SubList List<SubtitleInfo>
        }
        private void srt_charcode_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[5].Text = srt_charcode.SelectedItem.ToString();
            lv_subList.Select();

            SubList[lv_subList.SelectedIndices[0]].SrtCharCode = srt_charcode.SelectedItem.ToString(); // Update SubList List<SubtitleInfo>
        }
        private void srt_lang_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[4].Text = srt_lang.SelectedItem.ToString();
            lv_subList.Select();

            SubList[lv_subList.SelectedIndices[0]].SrtLang = srt_lang.SelectedItem.ToString(); // Update SubList List<SubtitleInfo>
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
            RemoveTrack();
        }

        // Functions
        private void SetNoDefault()
        {
            int c = 0;
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (item.SubItems[3].Text == "Yes")
                {
                    item.SubItems[3].Text = "No";
                    SubList[c].Default = "No";
                }
                c++;
            }
        }
        private void SetNoBurned()
        {
            int c = 0;
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (item.SubItems[2].Text == "Yes")
                {
                    item.SubItems[2].Text = "No";
                    SubList[c].Burned = "No";
                }
                c++;
            }
        }
        private void RemoveTrack()
        {
            // Remove the Item and reselect the control if the following conditions are met.
            if (lv_subList.SelectedItems.Count != 0)
            {
                // Record the current selected index.
                int currentPosition = lv_subList.SelectedIndices[0];
                int selectedInd = lv_subList.SelectedIndices[0];

                lv_subList.Items.RemoveAt(selectedInd);
                SubList.RemoveAt(selectedInd);

                // Now reslect the correct item and give focus to the list.
                if (lv_subList.Items.Count != 0)
                {
                    if (currentPosition <= (lv_subList.Items.Count - 1))
                        lv_subList.Items[currentPosition].Selected = true;
                    else if (currentPosition > (lv_subList.Items.Count - 1))
                        lv_subList.Items[lv_subList.Items.Count - 1].Selected = true;

                    lv_subList.Select();
                }
            }
        }

        // Public Functions
        public void Clear()
        {
            lv_subList.Items.Clear();
            SubList.Clear();
        }
        public void SmartClear()
        {
            /* Smart clear will only remove subtitle tracks that do not have an equivlent 
              for the new source / title which the user has selected. */
            throw new NotImplementedException();
        }
        public Boolean RequiresM4V()
        {
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (item.SubItems.Count != 5)
                    return true;

                if (item.SubItems[1].Text.Contains("(Text)"))
                    return true;
            }
            return false;
        }
        public void SetSubtitleTrackAuto()
        {
            // Handle Native Language and "Dub Foreign language audio" and "Use Foreign language audio and Subtitles" Options
            if (Properties.Settings.Default.NativeLanguage != "Any")
            {
                if (!Properties.Settings.Default.DubAudio) // We need to add a subtitle track if this is false.
                {
                    int i = 0;
                    foreach (object item in drp_subtitleTracks.Items)
                    {
                        if (item.ToString().Contains(Properties.Settings.Default.NativeLanguage))
                            drp_subtitleTracks.SelectedIndex = i;

                        i++;
                    }

                    btn_addSubTrack_Click(this, new EventArgs());
                }
            }
        }
        public void SetContainer(int value)
        {
            _fileContainer = value;
            Boolean trigger = false;
            if (_fileContainer != 1)
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
        public List<SubtitleInfo> GetSubtitleInfoList()
        {
            return SubList;
        }
    }

    public class SubtitleInfo
    {
        public string Track { get; set; }
        public string Forced { get; set; }
        public string Burned { get; set; }
        public string Default { get; set; }
        public string SrtLang { get; set; }
        public string SrtCharCode { get; set; }
        public int SrtOffset { get; set; }
        public string SrtPath { get; set; }
        public string SrtFileName { get; set; }

        public ListViewItem ListView
        {
            get
            {
                ListViewItem listTrack = new ListViewItem(Track);
                listTrack.SubItems.Add(Forced);
                listTrack.SubItems.Add(Burned);
                listTrack.SubItems.Add(Default);
                listTrack.SubItems.Add(SrtLang);
                listTrack.SubItems.Add(SrtCharCode);
                listTrack.SubItems.Add(SrtOffset.ToString());
                return listTrack;
            }
        }
    }
}