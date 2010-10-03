/*  Subtitles.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Windows.Forms;
    using Functions;

    using HandBrake.ApplicationServices.Model;

    using Model;

    /// <summary>
    /// The Subtitles Tab
    /// </summary>
    public partial class Subtitles : UserControl
    {
        #region Priavate Variables and Collections

        /// <summary>
        /// Map of languages to language codes
        /// </summary>
        private readonly IDictionary<string, string> langMap = new Dictionary<string, string>();

        /// <summary>
        /// A List of SRT Files
        /// </summary>
        private readonly IDictionary<string, string> srtFiles = new Dictionary<string, string>();

        /// <summary>
        /// The Subtitle List
        /// </summary>
        private readonly List<SubtitleInfo> subList = new List<SubtitleInfo>();

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="Subtitles"/> class.
        /// </summary>
        public Subtitles()
        {
            InitializeComponent();

            langMap = Main.MapLanguages();
            foreach (string key in langMap.Keys)
                srt_lang.Items.Add(key);

            srt_charcode.SelectedIndex = 28;
            srt_lang.SelectedIndex = 40;
        }

        #region Public Methods
        /// <summary>
        /// Gets the CLI Query for this panel
        /// </summary>
        /// <returns>A CliQuery string</returns>
        public string GetCliQuery
        {
            get
            {
                string query = string.Empty;
                if (lv_subList.Items.Count != 0) // If we have subtitle tracks
                {
                    // BitMap and CC's
                    string subtitleTracks = String.Empty;
                    string subtitleForced = String.Empty;
                    string subtitleBurn = String.Empty;
                    string subtitleDefault = String.Empty;

                    // SRT
                    string srtFile = String.Empty;
                    string srtCodeset = String.Empty;
                    string srtOffset = String.Empty;
                    string srtLang = String.Empty;
                    string srtDefault = String.Empty;
                    int srtCount = 0;
                    int subCount = 0;

                    foreach (SubtitleInfo item in subList)
                    {
                        string itemToAdd;

                        if (item.IsSrtSubtitle) // We have an SRT file
                        {
                            srtCount++; // SRT track id.

                            srtLang += srtLang == string.Empty ? langMap[item.SrtLang] : "," + langMap[item.SrtLang];
                            srtCodeset += srtCodeset == string.Empty ? item.SrtCharCode : "," + item.SrtCharCode;

                            if (item.Default)
                                srtDefault = srtCount.ToString();

                            itemToAdd = item.SrtPath;
                            srtFile += srtFile == string.Empty ? itemToAdd : "," + itemToAdd;

                            itemToAdd = item.SrtOffset.ToString();
                            srtOffset += srtOffset == string.Empty ? itemToAdd : "," + itemToAdd;
                        }
                        else // We have Bitmap or CC
                        {
                            subCount++;

                            // Find --subtitle <string>
                            if (item.Track.Contains("Foreign Audio Search"))
                                itemToAdd = "scan";
                            else
                            {
                                string[] tempSub = item.Track.Split(' ');
                                itemToAdd = tempSub[0];
                            }

                            subtitleTracks += subtitleTracks == string.Empty ? itemToAdd : "," + itemToAdd;

                            // Find --subtitle-forced
                            if (item.Forced)
                                subtitleForced += subtitleForced == string.Empty ? subCount.ToString() : "," + subCount;

                            // Find --subtitle-burn
                            if (item.Burned)
                                subtitleBurn = subCount.ToString();

                            // Find --subtitle-default
                            if (item.Default)
                                subtitleDefault = subCount.ToString();
                        }
                    }

                    // Build The CLI Subtitles Query
                    if (subtitleTracks != string.Empty)
                    {
                        query += " --subtitle " + subtitleTracks;

                        if (subtitleForced != string.Empty)
                            query += " --subtitle-forced=" + subtitleForced;
                        if (subtitleBurn != string.Empty)
                            query += " --subtitle-burn=" + subtitleBurn;
                        if (subtitleDefault != string.Empty)
                            query += " --subtitle-default=" + subtitleDefault;
                    }

                    if (srtFile != string.Empty) // SRTs
                    {
                        query += " --srt-file " + "\"" + srtFile + "\"";

                        if (srtCodeset != string.Empty)
                            query += " --srt-codeset " + srtCodeset;
                        if (srtOffset != string.Empty)
                            query += " --srt-offset " + srtOffset;
                        if (srtLang != string.Empty)
                            query += " --srt-lang " + srtLang;
                        if (srtDefault != string.Empty)
                            query += " --srt-default=" + srtDefault;
                    }
                }
                return query;
            }
        }

        /// <summary>
        /// Checks of the current settings will require the m4v file extension
        /// </summary>
        /// <returns>True if Yes</returns>
        public bool RequiresM4V()
        {
            return this.subList.Any(track => track.SubtitleType != SubtitleType.VobSub);
        }

        /// <summary>
        /// Automatically setup the subtitle tracks.
        /// This handles the automatic setup of subitles which the user can control from the program options
        /// </summary>
        /// <param name="subs">List of Subtitles</param>
        public void SetSubtitleTrackAuto(object[] subs)
        {
            drp_subtitleTracks.Items.Clear();
            drp_subtitleTracks.Items.Add("Foreign Audio Search (Bitmap)");
            drp_subtitleTracks.Items.AddRange(subs);
            drp_subtitleTracks.SelectedIndex = 0;
            Clear();

            this.AutomaticSubtitleSelection();
        }

        /// <summary>
        /// Automatic Subtitle Selection based on user preferences.
        /// </summary>
        public void AutomaticSubtitleSelection()
        {
            // Handle Native Language and "Dub Foreign language audio" and "Use Foreign language audio and Subtitles" Options
            if (Properties.Settings.Default.NativeLanguage != "Any")
            {
                if (Properties.Settings.Default.DubMode != 1) // We need to add a subtitle track if this is false.
                {
                    foreach (object item in drp_subtitleTracks.Items)
                    {
                        if (item.ToString().Contains(Properties.Settings.Default.NativeLanguage))
                        {
                            drp_subtitleTracks.SelectedItem = item;
                            BtnAddSubTrackClick(this, new EventArgs());
                        }
                    }

                    if (drp_subtitleTracks.SelectedIndex == 0 && Properties.Settings.Default.useClosedCaption)
                    {
                        foreach (object item in drp_subtitleTracks.Items)
                        {
                            if (item.ToString().Contains("Closed"))
                            {
                                drp_subtitleTracks.SelectedItem = item;
                                BtnAddSubTrackClick(this, new EventArgs());
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Clear the Subtitle List
        /// </summary>
        public void Clear()
        {
            lv_subList.Items.Clear();
            subList.Clear();
        }
        #endregion

        #region Primary Controls

        /// <summary>
        /// Add a Subtitle track.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnAddSubTrackClick(object sender, EventArgs e)
        {
            // Logic
            string forcedVal = check_forced.CheckState == CheckState.Checked ? "Yes" : "No";
            string defaultSub = check_default.CheckState == CheckState.Checked ? "Yes" : "No";
            string burnedVal = check_burned.CheckState == CheckState.Checked &&
                               (drp_subtitleTracks.Text.Contains("(VOBSUB)") || drp_subtitleTracks.Text.Contains("(SSA)"))
                                   ? "Yes"
                                   : "No";
            string srtCode = "-", srtLangVal = "-", srtPath = "-", srtFile = "-";
            int srtOffsetMs = 0;

            if (drp_subtitleTracks.SelectedItem.ToString().Contains(".srt"))
            {
                srtFiles.TryGetValue(drp_subtitleTracks.SelectedItem.ToString(), out srtPath);
                srtFile = drp_subtitleTracks.SelectedItem.ToString();
                srtLangVal = srt_lang.SelectedItem.ToString();
                srtCode = srt_charcode.SelectedItem.ToString();
                srtOffsetMs = (int)srt_offset.Value;
                if (defaultSub == "Yes") this.SetDefaultToOffForAllSRTTracks();
            }
            else
            {
                if (defaultSub == "Yes") this.SetDefaultToOffForAllTracks();
                if (burnedVal == "Yes") this.SetBurnedToOffForAllTracks();
            }

            string trackName = (drp_subtitleTracks.SelectedItem.ToString().Contains(".srt"))
                                   ? srtLangVal + " (" + srtFile + ")"
                                   : drp_subtitleTracks.SelectedItem.ToString();

            SubtitleInfo track = new SubtitleInfo
                                     {
                                         Track = trackName,
                                         Forced = check_forced.Checked,
                                         Burned = check_burned.Checked,
                                         Default = check_default.Checked,
                                         SrtLang = srtLangVal,
                                         SrtCharCode = srtCode,
                                         SrtOffset = srtOffsetMs,
                                         SrtPath = srtPath,
                                         SrtFileName = srtFile
                                     };

            lv_subList.Items.Add(track.ListView);
            subList.Add(track);
        }

        /// <summary>
        /// Import an SRT Subtitle Track
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnSrtAddClick(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog() != DialogResult.OK)
                return;

            if (srtFiles.ContainsKey(Path.GetFileName(openFileDialog.FileName)))
            {
                MessageBox.Show(
                    "A Subtitle track with the same name has already been imported.",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                return;
            }

            drp_subtitleTracks.Items.Add(Path.GetFileName(openFileDialog.FileName));
            drp_subtitleTracks.SelectedItem = Path.GetFileName(openFileDialog.FileName);
            srtFiles.Add(Path.GetFileName(openFileDialog.FileName), openFileDialog.FileName);
        }

        /// <summary>
        /// Remove a subtitle track
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnRemoveSubTrackClick(object sender, EventArgs e)
        {
            this.RemoveSelectedTrack();
        }

        /// <summary>
        /// The Subtitle List selected index has changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void LbSubListSelectedIndexChanged(object sender, EventArgs e)
        {
            // Set the dropdown controls based on the selected item in the List.
            if (lv_subList.Items.Count != 0 && lv_subList.SelectedIndices.Count != 0)
            {
                SubtitleInfo track = subList[lv_subList.SelectedIndices[0]];

                int c = 0;
                if (lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text.ToLower().Contains(".srt"))
                // We have an SRT
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
                    check_default.CheckState = track.Default ? CheckState.Checked : CheckState.Unchecked;
                    SubGroupBox.Text = "Selected Track: " + track.Track;
                }
                else // We have Bitmap/CC
                {
                    foreach (var item in drp_subtitleTracks.Items)
                    {
                        if (item.ToString() == track.Track)
                            drp_subtitleTracks.SelectedIndex = c;
                        c++;
                    }
                    check_forced.CheckState = track.Forced ? CheckState.Checked : CheckState.Unchecked;
                    check_burned.CheckState = track.Burned ? CheckState.Checked : CheckState.Unchecked;
                    check_default.CheckState = track.Default ? CheckState.Checked : CheckState.Unchecked;
                    SubGroupBox.Text = "Selected Track: " +
                                       lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text;
                }
            }
            else
                SubGroupBox.Text = "Selected Track: None (Click \"Add\" to add another track to the list)";
        }

        #endregion

        #region Subtitle Controls

        /// <summary>
        /// Handle the Subtitle track dropdown changed event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void DrpSubtitleTracksSelectedIndexChanged(object sender, EventArgs e)
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

            if (drp_subtitleTracks.SelectedItem.ToString().Contains(".srt"))
            {
                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text = srt_lang.SelectedItem + "(" +
                                                                                   drp_subtitleTracks.SelectedItem + ")";
                lv_subList.Select();

                string srtPath;
                srtFiles.TryGetValue(drp_subtitleTracks.SelectedItem.ToString(), out srtPath);
                subList[lv_subList.SelectedIndices[0]].SrtPath = srtPath;
            }
            else
            {
                lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text =
                    drp_subtitleTracks.SelectedItem.ToString();
                lv_subList.Select();
            }

            subList[lv_subList.SelectedIndices[0]].Track = drp_subtitleTracks.SelectedItem.ToString();
        }

        /// <summary>
        /// Foreced Subtitles checkbox changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void CheckForcedCheckedChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[1].Text = check_forced.Checked ? "Yes" : "No";
            lv_subList.Select();

            subList[lv_subList.SelectedIndices[0]].Forced = check_forced.Checked;
            // Update SubList List<SubtitleInfo> 
        }

        /// <summary>
        /// Burned in subtitles checkbox changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void CheckBurnedCheckedChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            if (check_burned.Checked) // Make sure we only have 1 burned track
                this.SetBurnedToOffForAllTracks();

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[2].Text = check_burned.Checked ? "Yes" : "No";
            lv_subList.Select();

            subList[lv_subList.SelectedIndices[0]].Burned = check_burned.Checked;
            // Update SubList List<SubtitleInfo> 
        }

        /// <summary>
        /// Default Subitle track check changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void CheckDefaultCheckedChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            if (check_default.Checked) // Make sure we only have 1 default track
                if (lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[0].Text.Contains(".srt"))
                    this.SetDefaultToOffForAllSRTTracks();
                else
                    this.SetDefaultToOffForAllTracks();

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[3].Text = check_default.Checked ? "Yes" : "No";
            lv_subList.Select();

            subList[lv_subList.SelectedIndices[0]].Default = check_default.Checked;
            // Update SubList List<SubtitleInfo>
        }

        /// <summary>
        /// Srt Offset Numeric Control Changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void SrtOffsetValueChanged(object sender, EventArgs e)
        {
            // Update an item in the  list if required.
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0)
                return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[6].Text = srt_offset.Value.ToString();
            lv_subList.Select();

            subList[lv_subList.SelectedIndices[0]].SrtOffset = (int)srt_offset.Value;
            // Update SubList List<SubtitleInfo>
        }

        /// <summary>
        /// The SRT Charcode changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void SrtCharcodeSelectedIndexChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[5].Text = srt_charcode.SelectedItem.ToString();
            lv_subList.Select();

            subList[lv_subList.SelectedIndices[0]].SrtCharCode = srt_charcode.SelectedItem.ToString();
            // Update SubList List<SubtitleInfo>
        }

        /// <summary>
        /// The Srt Language code changed
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void SrtLangSelectedIndexChanged(object sender, EventArgs e)
        {
            if (lv_subList.Items.Count == 0 || lv_subList.SelectedIndices.Count == 0) return;

            lv_subList.Items[lv_subList.SelectedIndices[0]].SubItems[4].Text = srt_lang.SelectedItem.ToString();
            lv_subList.Select();

            subList[lv_subList.SelectedIndices[0]].SrtLang = srt_lang.SelectedItem.ToString();
            // Update SubList List<SubtitleInfo>
        }

        #endregion

        #region Right Click Menu

        /// <summary>
        /// Move an item in the subtitle list up
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuMoveupClick(object sender, EventArgs e)
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

        /// <summary>
        /// Move an item in the subtitle list down
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuMovedownClick(object sender, EventArgs e)
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

        /// <summary>
        /// Remove a selected subtitle track.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuRemoveClick(object sender, EventArgs e)
        {
            this.RemoveSelectedTrack();
        }

        #endregion

        #region Helpers
        /// <summary>
        /// Set all Non SRT tracks to Default = NO
        /// </summary>
        private void SetDefaultToOffForAllTracks()
        {
            int c = 0;
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (subList[c].SrtPath == "-")
                {
                    if (item.SubItems[3].Text == "Yes")
                    {
                        item.SubItems[3].Text = "No";
                        subList[c].Default = false;
                    }
                }
                c++;
            }
        }

        /// <summary>
        /// Set all subtitle tracks so that they have no default.
        /// </summary>
        private void SetDefaultToOffForAllSRTTracks()
        {
            int c = 0;
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (!subList[c].IsSrtSubtitle)
                {
                    if (item.SubItems[3].Text == "Yes")
                    {
                        item.SubItems[3].Text = "No";
                        subList[c].Default = false;
                    }
                }
                c++;
            }
        }

        /// <summary>
        /// Set all tracks to Burned = No
        /// </summary>
        private void SetBurnedToOffForAllTracks()
        {
            int c = 0;
            foreach (ListViewItem item in lv_subList.Items)
            {
                if (item.SubItems[2].Text == "Yes")
                {
                    item.SubItems[2].Text = "No";
                    subList[c].Burned = false;
                }
                c++;
            }
        }

        /// <summary>
        /// Remove a selected track
        /// </summary>
        private void RemoveSelectedTrack()
        {
            // Remove the Item and reselect the control if the following conditions are met.
            if (lv_subList.SelectedItems.Count != 0)
            {
                // Record the current selected index.
                int currentPosition = lv_subList.SelectedIndices[0];
                int selectedInd = lv_subList.SelectedIndices[0];

                lv_subList.Items.RemoveAt(selectedInd);
                subList.RemoveAt(selectedInd);

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
        #endregion
    }
}