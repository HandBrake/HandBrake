/*  AudioPanel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Globalization;
    using System.Linq;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Utilities;

    using Handbrake.ToolWindows;

    using AudioTrack = HandBrake.ApplicationServices.Model.Encoding.AudioTrack;

    /// <summary>
    /// The AudioPanel Control
    /// </summary>
    public partial class AudioPanel : UserControl
    {
        /**
         * TODO - There is a lot of old code in here that access the datagrid that can be refactored to work with 
         *        the new AudioTrack BindingList.
         */

        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);
        private const string AC3Passthru = "AC3 Passthru";
        private const string DTSPassthru = "DTS Passthru";
        AdvancedAudio advancedAudio = new AdvancedAudio();

        private readonly BindingList<AudioTrack> audioTracks = new BindingList<AudioTrack>();

        #region Constructor and Events

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioPanel"/> class. 
        /// </summary>
        public AudioPanel()
        {
            InitializeComponent();

            this.audioList.AutoGenerateColumns = false;
            this.audioList.DataSource = audioTracks;

            drp_audioMix.SelectedItem = "Dolby Pro Logic II";
            drp_audioSample.SelectedIndex = 1;

           
        }

        /// <summary>
        /// The audio list has changed
        /// </summary>
        public event EventHandler AudioListChanged;

        #endregion

        #region Properties

        /// <summary>
        /// Gets the AudioTracks Collection
        /// </summary>
        public IEnumerable<AudioTrack> AudioTracks
        {
            get
            {
                return this.audioTracks;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Set the File Container. This funciton is used to limit the available options for each file container.
        /// </summary>
        /// <param name="path">
        /// the file path
        /// </param>
        public void SetContainer(string path)
        {
            string oldval = drp_audioEncoder.Text;

            drp_audioEncoder.Items.Clear();
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDescription(AudioEncoder.Faac));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDescription(AudioEncoder.Lame));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDescription(AudioEncoder.Ac3Passthrough));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDescription(AudioEncoder.Ac3));

            if (path.Contains("MKV"))
            {
                drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDescription(AudioEncoder.DtsPassthrough));
                drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDescription(AudioEncoder.Vorbis));
            }

            if (!drp_audioEncoder.Text.Contains(oldval))
                drp_audioEncoder.SelectedIndex = 0;
            else
                drp_audioEncoder.SelectedItem = oldval;
        }

        /// <summary>
        /// Checks if the settings used required the .m4v (rather than .mp4) extension
        /// </summary>
        /// <returns>True if m4v is required</returns>
        public bool RequiresM4V()
        {
            return true;
        }

        /// <summary>
        /// Load an arraylist of AudioTrack items into the list.
        /// </summary>
        /// <param name="audioTracks">List of audio tracks</param>
        public void LoadTracks(List<AudioTrack> audioTracks)
        {
            ClearAudioList();

            if (audioTracks == null)
                return;

            foreach (AudioTrack track in audioTracks)
            {
                this.audioTracks.Add(track);
            }

            this.AutomaticTrackSelection();

            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
        }

        /// <summary>
        /// Set the Track list dropdown from the parsed title captured during the scan
        /// </summary>
        /// <param name="selectedTitle">The selected title</param>
        /// <param name="preset">A preset</param>
        public void SetTrackListFromPreset(Title selectedTitle, Preset preset)
        {
            if (selectedTitle.AudioTracks.Count == 0)
            {
                audioList.Rows.Clear();
                drp_audioTrack.Items.Clear();
                drp_audioTrack.Items.Add("None Found");
                drp_audioTrack.SelectedIndex = 0;
                return;
            }

            // Setup the Audio track source dropdown with the new audio tracks.
            drp_audioTrack.Items.Clear();
            drp_audioTrack.Items.AddRange(selectedTitle.AudioTracks.ToArray());

            // Add any tracks the preset has, if there is a preset and no audio tracks in the list currently
            if (audioList.Rows.Count == 0 && preset != null)
            {
                EncodeTask parsed = QueryParserUtility.Parse(preset.Query);
                foreach (AudioTrack audioTrack in parsed.AudioTracks)
                {
                    this.audioTracks.Add(audioTrack);
                }
            }

            this.AutomaticTrackSelection();
        }

        #endregion

        #region Control and ListView

        /// <summary>
        /// one of the controls has changed. Event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ControlChanged(object sender, EventArgs e)
        {
            if (audioList.SelectedRows.Count == 0)
            {
                return;
            }

            AudioTrack track = audioList.SelectedRows[0].DataBoundItem as AudioTrack;
            if (track == null)
            {
                return;
            }

            Control ctl = (Control)sender;

            switch (ctl.Name)
            {
                case "drp_audioTrack":
                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0)
                    {
                        track.SourceTrack = drp_audioTrack.Text;

                        // If the track isn't AC3, and the encoder is, change it.
                        if (track.Encoder == AudioEncoder.Ac3Passthrough && !track.SourceTrack.Contains("(AC3)"))
                        {
                            // Switch to AAC
                            drp_audioEncoder.SelectedIndex = 0;
                        }

                        // If the track isn't DTS, and the encoder is, change it.
                        if (track.Encoder == AudioEncoder.DtsPassthrough && !track.SourceTrack.Contains("DTS"))
                        {
                            // Switch to AAC
                            drp_audioEncoder.SelectedIndex = 0;
                        }
                    }
                    break;
                case "drp_audioEncoder":
                    SetMixDown();
                    SetBitrate();

                    // Configure the widgets with values
                    if (drp_audioEncoder.Text.Contains(AC3Passthru) || drp_audioEncoder.Text.Contains(DTSPassthru))
                    {
                        drp_audioMix.Enabled = drp_audioBitrate.Enabled = drp_audioSample.Enabled = tb_drc.Enabled = false;
                    }
                    else
                    {
                        drp_audioMix.Enabled = drp_audioBitrate.Enabled = drp_audioSample.Enabled = tb_drc.Enabled = true;
                    }

                    // Update an item in the Audio list if required.
                    track.Encoder = EnumHelper<AudioEncoder>.GetValue(drp_audioEncoder.Text);
                    break;
                case "drp_audioMix":
                    SetBitrate();

                    track.MixDown = EnumHelper<Mixdown>.GetValue(drp_audioMix.Text);

                    break;
                case "drp_audioSample":

                    double samplerate;
                    double.TryParse(drp_audioSample.Text, out samplerate);
                    track.SampleRate = samplerate;
                    break;
                case "drp_audioBitrate":
                    // Update an item in the Audio list if required.
                    int bitrate;
                    int.TryParse(drp_audioBitrate.Text, out bitrate);

                    track.Bitrate = bitrate;
                    break;
                case "tb_drc":
                    double value;
                    if (tb_drc.Value == 0) value = 0;
                    else
                        value = ((tb_drc.Value - 1) / 10.0) + 1;

                    lbl_drc.Text = value.ToString();
                    track.DRC = value;
                    audioList.Select();
                    audioList.Refresh();
                    break;
            }

            audioList.Refresh();

            if (audioList.SelectedRows.Count == 1)
            {
                AudioTrack item = audioList.SelectedRows[0].DataBoundItem as AudioTrack;
                if (item != null) lbl_audioTrack.Text = track.SourceTrack;
            }
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
                AudioTrack track = audioList.SelectedRows[0].DataBoundItem as AudioTrack;
                if (track != null)
                {
                    drp_audioTrack.SelectedItem = track.SourceTrack;
                    drp_audioEncoder.SelectedItem = EnumHelper<AudioEncoder>.GetDescription(track.Encoder);
                    drp_audioMix.SelectedItem = EnumHelper<Mixdown>.GetDescription(track.MixDown);
                    drp_audioSample.SelectedItem = track.SampleRate;
                    drp_audioBitrate.SelectedItem = track.Bitrate;
                    double drcValue = 0;
                    int drcCalculated;
                    if (track.DRC != 0)
                        drcValue = ((track.DRC * 10) + 1) - 10;
                    int.TryParse(drcValue.ToString(Culture), out drcCalculated);
                    tb_drc.Value = drcCalculated;
                    lbl_drc.Text = track.DRC.ToString();

                    lbl_audioTrack.Text = track.SourceTrack;

                    // Set the Advanced Control.
                    if (!advancedAudio.IsDisposed)
                        advancedAudio.Track = track;
                }
            }
            else
                lbl_audioTrack.Text = "(Click \"Add Track\" to add)";

        }

        #endregion

        #region Track Controls

        /// <summary>
        /// The Add Audio Track button event handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void AddAudioTrack_Click(object sender, EventArgs e)
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

            // Get Some Values
            double drcValue = 0;
            if (tb_drc.Value != 0)
                drcValue = ((tb_drc.Value - 1) / 10.0) + 1;

            int bitrate;
            double samplerate;

            int.TryParse(drp_audioBitrate.Text, out bitrate);
            double.TryParse(drp_audioSample.Text, out samplerate);

            // Create the Model
            AudioTrack track = new AudioTrack
                {
                    SourceTrack = this.drp_audioTrack.Text,
                    Encoder = EnumHelper<AudioEncoder>.GetValue(this.drp_audioEncoder.Text),
                    MixDown = EnumHelper<Mixdown>.GetValue(this.drp_audioMix.Text),
                    SampleRate = samplerate,
                    Bitrate = bitrate,
                    Gain = 0,
                    DRC = drcValue
                };

            this.audioTracks.Add(track);

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
        private void RemoveAudioTrack_Click(object sender, EventArgs e)
        {
            RemoveTrack();
        }

        #endregion

        #region Audio List Menu

        /// <summary>
        /// The Audio List Move Up menu item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void AudioList_moveup_Click(object sender, EventArgs e)
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
        private void AudioList_movedown_Click(object sender, EventArgs e)
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
        private void AudioList_remove_Click(object sender, EventArgs e)
        {
            RemoveTrack();
        }

        #endregion

        #region Private Functions

        /// <summary>
        /// Attempt to automatically select the correct audio tracks based on the users settings.
        /// </summary>
        private void AutomaticTrackSelection()
        {
            // Handle Native Language and "Dub Foreign language audio" and "Use Foreign language audio and Subtitles" Options
            if (Properties.Settings.Default.NativeLanguage == "Any")
            {
                drp_audioTrack.SelectedIndex = 0;
                foreach (AudioTrack track in this.audioTracks)
                {
                    if (this.drp_audioTrack.SelectedItem != null)
                    {
                        track.SourceTrack = this.drp_audioTrack.SelectedItem.ToString();
                    }
                }
            }
            else
            {
                int mode = Properties.Settings.Default.DubMode;
                switch (mode)
                {
                    case 1:
                    case 3:
                        // Dub Foreign Language Audio 
                        // Select the prefered language audio, or the first track if it doesn't exist.
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
                            foreach (AudioTrack track in this.audioTracks)
                                track.SourceTrack = drp_audioTrack.SelectedItem.ToString();
                        else
                        {
                            drp_audioTrack.SelectedIndex = 0;
                            if (drp_audioTrack.SelectedItem != null)
                                foreach (AudioTrack track in this.audioTracks)
                                    track.SourceTrack = drp_audioTrack.SelectedItem.ToString();
                        }

                        break;
                    case 2:
                    default:
                        // Select the first track which is hopefully the default and foreign track.
                        drp_audioTrack.SelectedIndex = 0;

                        if (drp_audioTrack.SelectedItem != null)
                            foreach (AudioTrack track in this.audioTracks)
                                track.SourceTrack = drp_audioTrack.SelectedItem.ToString();
                        break;
                }
            }
        }

        /// <summary>
        /// Clear the audio list
        /// </summary>
        private void ClearAudioList()
        {
            this.audioTracks.Clear();

            if (this.AudioListChanged != null)
                this.AudioListChanged(this, new EventArgs());
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
        /// Set the bitrate dropdown
        /// </summary>
        private void SetBitrate()
        {
            int max = 0;
            string defaultRate = "160";

            // Remove defaults
            drp_audioBitrate.Items.Remove("Auto");
            drp_audioBitrate.Items.Remove("192");
            drp_audioBitrate.Items.Remove("224");
            drp_audioBitrate.Items.Remove("256");
            drp_audioBitrate.Items.Remove("320");
            drp_audioBitrate.Items.Remove("384");
            drp_audioBitrate.Items.Remove("448");
            drp_audioBitrate.Items.Remove("640");
            drp_audioBitrate.Items.Remove("768");

            // Find Max and Defaults based on encoders
            switch (drp_audioEncoder.Text)
            {
                case "AAC (faac)":
                    max = drp_audioMix.Text.Contains("6 Channel") ? 768 : 320;
                    defaultRate = "160";
                    break;
                case "MP3 (lame)":
                    max = 320;
                    defaultRate = "160";
                    break;
                case "Vorbis (vorbis)":
                    defaultRate = "160";
                    max = 384;
                    break;
                case "AC3 (ffmpeg)":
                    defaultRate = "640";
                    max = 640;
                    break;
                case AC3Passthru:
                    drp_audioBitrate.Items.Add("Auto");
                    defaultRate = "Auto";
                    drp_audioSample.SelectedItem = "Auto";
                    break;
                case DTSPassthru:
                    drp_audioBitrate.Items.Add("Auto");
                    defaultRate = "Auto";
                    drp_audioSample.SelectedItem = "Auto";
                    break;
            }

            // Re-add appropiate options
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

            if (max >= 640)
            {
                drp_audioBitrate.Items.Add("448");
                drp_audioBitrate.Items.Add("640");
            }

            if (max == 768)
            {
                drp_audioBitrate.Items.Add("768");
            }

            // Set the default bit-rate
            drp_audioBitrate.SelectedItem = defaultRate;
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
            drp_audioMix.Items.Add(AC3Passthru);
            drp_audioMix.Items.Add(DTSPassthru);

            drp_audioMix.SelectedItem = "Dolby Pro Logic II";

            switch (drp_audioEncoder.Text)
            {
                case "AAC (faac)":
                    drp_audioMix.Items.Remove(AC3Passthru);
                    drp_audioMix.Items.Remove(DTSPassthru);
                    break;
                case "MP3 (lame)":
                    drp_audioMix.Items.Remove("6 Channel Discrete");
                    drp_audioMix.Items.Remove(AC3Passthru);
                    drp_audioMix.Items.Remove(DTSPassthru);
                    break;
                case "Vorbis (vorbis)":
                    drp_audioMix.Items.Remove(AC3Passthru);
                    drp_audioMix.Items.Remove(DTSPassthru);
                    break;
                case "AC3 (ffmpeg)":
                    drp_audioMix.Items.Remove(AC3Passthru);
                    drp_audioMix.Items.Remove(DTSPassthru);
                    break;
                case "AC3 Passthru":
                    drp_audioMix.SelectedItem = AC3Passthru;
                    break;
                case "DTS Passthru":
                    drp_audioMix.SelectedItem = DTSPassthru;
                    break;
            }
        }

        #endregion

        private void btn_AdvancedAudio_Click(object sender, EventArgs e)
        {
            if (advancedAudio.IsDisposed)
            {
                advancedAudio = new AdvancedAudio { Track = this.audioList.SelectedRows[0].DataBoundItem as AudioTrack };
            }

            advancedAudio.Show();
        }

       
    }
}