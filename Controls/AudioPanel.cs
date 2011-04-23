/*  AudioPanel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Drawing;
    using System.Linq;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Utilities;

    using Handbrake.ToolWindows;

    /// <summary>
    /// The AudioPanel Control
    /// </summary>
    public partial class AudioPanel : UserControl
    {
        #region Private Variables
        private readonly BindingList<AudioTrack> audioTracks = new BindingList<AudioTrack>();
        private const string AC3Passthru = "AC3 Passthru";
        private const string DTSPassthru = "DTS Passthru";
        private AdvancedAudio advancedAudio = new AdvancedAudio();
        #endregion

        #region Constructor and Events

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioPanel"/> class. 
        /// </summary>
        public AudioPanel()
        {
            InitializeComponent();

            this.ScannedTracks = new BindingList<Audio>
                {
                    Audio.NoneFound 
                };

            this.audioList.AutoGenerateColumns = false;
            this.audioList.DataSource = audioTracks;

            drp_audioMix.SelectedItem = "Dolby Pro Logic II";
            drp_audioSample.SelectedIndex = 1;

            drp_audioTrack.DataSource = this.ScannedTracks;
        }

        /// <summary>
        /// The audio list has changed
        /// </summary>
        public event EventHandler AudioListChanged;

        #endregion

        #region Properties

        public BindingList<Audio> ScannedTracks { get; set; }

        /// <summary>
        /// Gets the AudioTracks Collection
        /// </summary>
        public BindingList<AudioTrack> AudioTracks
        {
            get
            {
                return this.audioTracks;
            }
        }

        /// <summary>
        /// Gets the Estimated Bitrate for Tracks and Passthru Tracks
        /// Returns a Size object (Encoded Tracks, Passthru Trask)
        /// </summary>
        public Size EstimatedBitrate
        {
            get
            {
                int encodedTracks = 0;
                int passthruTracks = 0;
                foreach (AudioTrack track in this.AudioTracks)
                {
                    if (track.Encoder == AudioEncoder.Ac3Passthrough || track.Encoder == AudioEncoder.DtsPassthrough)
                    {
                        passthruTracks += track.ScannedTrack.Bitrate;
                    } 
                    else
                    {
                        encodedTracks += track.Bitrate;
                    }
                }

                return new Size(encodedTracks, passthruTracks);
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
            return this.AudioTracks.Any(item => item.Encoder == AudioEncoder.Ac3Passthrough || item.Encoder == AudioEncoder.DtsPassthrough || item.Encoder == AudioEncoder.Ac3);
        }

        /// <summary>
        /// Load an arraylist of AudioTrack items into the list.
        /// </summary>
        /// <param name="tracks">List of audio tracks</param>
        public void LoadTracks(List<AudioTrack> tracks)
        {
            ClearAudioList();

            if (tracks == null)
                return;

            foreach (AudioTrack track in tracks)
            {
                if (track.Encoder == AudioEncoder.Ac3Passthrough)
                {
                    track.MixDown = HandBrake.ApplicationServices.Model.Encoding.Mixdown.Ac3Passthrough;
                }

                if (track.Encoder == AudioEncoder.DtsPassthrough)
                {
                    track.MixDown = HandBrake.ApplicationServices.Model.Encoding.Mixdown.DtsPassthrough;
                }

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
                this.ScannedTracks.Clear();
                this.ScannedTracks.Add(Audio.NoneFound);
                this.drp_audioTrack.Refresh();
                drp_audioTrack.SelectedIndex = 0;
                return;
            }

            // Setup the Audio track source dropdown with the new audio tracks.
            this.ScannedTracks.Clear();
            this.drp_audioTrack.SelectedItem = null;
            foreach (var item in selectedTitle.AudioTracks)
            {
                this.ScannedTracks.Add(item);
            }

            drp_audioTrack.SelectedItem = this.ScannedTracks.FirstOrDefault();
            this.drp_audioTrack.Refresh();

            // Add any tracks the preset has, if there is a preset and no audio tracks in the list currently
            if (audioList.Rows.Count == 0 && preset != null)
            {
                EncodeTask parsed = QueryParserUtility.Parse(preset.Query);
                foreach (AudioTrack audioTrack in parsed.AudioTracks)
                {
                    audioTrack.ScannedTrack = drp_audioTrack.SelectedItem as Audio;
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
                    if (audioList.Rows.Count != 0 && audioList.SelectedRows.Count != 0 && drp_audioTrack.SelectedItem != null)
                    {
                        track.ScannedTrack = drp_audioTrack.SelectedItem as Audio;

                        // If the track isn't AC3, and the encoder is, change it.
                        if (track.Encoder == AudioEncoder.Ac3Passthrough && !track.ScannedTrack.Format.Contains("AC3"))
                        {
                            // Switch to AAC
                            drp_audioEncoder.SelectedIndex = 0;
                        }

                        // If the track isn't DTS, and the encoder is, change it.
                        if (track.Encoder == AudioEncoder.DtsPassthrough && !track.ScannedTrack.Format.Contains("DTS"))
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
                        drp_audioMix.Enabled = drp_audioBitrate.Enabled = drp_audioSample.Enabled = btn_AdvancedAudio.Enabled = false;
                        track.Gain = 0;
                        track.DRC = 0;
                    }
                    else
                    {
                        drp_audioMix.Enabled = drp_audioBitrate.Enabled = drp_audioSample.Enabled = btn_AdvancedAudio.Enabled = true;
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
            }

            audioList.Refresh();
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
                    drp_audioTrack.SelectedItem = track.ScannedTrack;
                    drp_audioEncoder.SelectedItem = EnumHelper<AudioEncoder>.GetDescription(track.Encoder);
                    drp_audioMix.SelectedItem = EnumHelper<Mixdown>.GetDescription(track.MixDown);
                    drp_audioSample.SelectedItem = track.SampleRate;
                    drp_audioBitrate.SelectedItem = track.Bitrate;

                    // Set the Advanced Control.
                    if (!advancedAudio.IsDisposed)
                        advancedAudio.Track = track;
                }
            }
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
                    "Your source appears to have no audio tracks, or no tracks in a format that HandBrake supports.",
                    "Warning",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                return;
            }

            // Get Some Values
            int bitrate;
            double samplerate;

            int.TryParse(drp_audioBitrate.Text, out bitrate);
            double.TryParse(drp_audioSample.Text, out samplerate);

            // Create the Model
            AudioTrack track = new AudioTrack
                {
                    ScannedTrack = this.drp_audioTrack.SelectedItem as Audio,
                    Encoder = EnumHelper<AudioEncoder>.GetValue(this.drp_audioEncoder.Text),
                    MixDown = EnumHelper<Mixdown>.GetValue(this.drp_audioMix.Text),
                    SampleRate = samplerate,
                    Bitrate = bitrate,
                    Gain = 0,
                    DRC = 0,
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

        /// <summary>
        /// Move to Top
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The Event Args</param>
        private void audioList_MoveToTop_Click(object sender, EventArgs e)
        {
            MoveTo(true);
        }

        /// <summary>
        /// Move to Bottom
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The Event Args</param>
        private void audioList_MoveToBottom_Click(object sender, EventArgs e)
        {
            this.MoveTo(false);
        }

        #endregion

        #region Private Functions

        /// <summary>
        /// Attempt to automatically select the correct audio tracks based on the users settings.
        /// </summary>
        private void AutomaticTrackSelection()
        {
            if (drp_audioTrack.SelectedItem.ToString() == Audio.NoneFound.Description)
            {
                this.AudioTracks.Clear();
                return;
            }

            // Handle Native Language and "Dub Foreign language audio" and "Use Foreign language audio and Subtitles" Options
            if (Properties.Settings.Default.NativeLanguage == "Any")
            {
                drp_audioTrack.SelectedIndex = 0;
                foreach (AudioTrack track in this.audioTracks)
                {
                    if (this.drp_audioTrack.SelectedItem != null)
                    {
                        track.ScannedTrack = this.drp_audioTrack.SelectedItem as Audio;
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
                                track.ScannedTrack =
                                    drp_audioTrack.SelectedItem as Audio;
                        else
                        {
                            drp_audioTrack.SelectedIndex = 0;
                            if (drp_audioTrack.SelectedItem != null)
                                foreach (AudioTrack track in this.audioTracks)
                                    track.ScannedTrack = drp_audioTrack.SelectedItem as Audio;
                        }

                        break;
                    case 2:
                    default:
                        // Select the first track which is hopefully the default and foreign track.
                        drp_audioTrack.SelectedIndex = 0;

                        if (drp_audioTrack.SelectedItem != null)
                            foreach (AudioTrack track in this.audioTracks)
                                track.ScannedTrack = drp_audioTrack.SelectedItem as Audio;
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
            AudioTrack track = item.DataBoundItem as AudioTrack;
            int index = item.Index;

            if (up) index--;
            else index++;

            if (index < audioList.Rows.Count || (audioList.Rows.Count > index && index >= 0))
            {
                this.AudioTracks.Remove(track);
                this.audioTracks.Insert(index, track);
                this.audioList.ClearSelection();
                this.audioList.Rows[index].Selected = true;
            }
        }

        private void MoveTo(bool top)
        {
            if (audioList.SelectedRows.Count == 0) return;

            DataGridViewRow item = audioList.SelectedRows[0];
            AudioTrack track = item.DataBoundItem as AudioTrack;
            int index = item.Index;

            if (top) index = 0;
            else index = this.audioList.Rows.Count - 1;

            if (index < audioList.Rows.Count || (audioList.Rows.Count > index && index >= 0))
            {
                this.AudioTracks.Remove(track);
                this.audioTracks.Insert(index, track);
                this.audioList.ClearSelection();
                this.audioList.Rows[index].Selected = true;
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
            if (audioList.SelectedRows.Count == 0)
            {
                MessageBox.Show(
                    "Please select an audio track.", "No Track Selected", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            if (advancedAudio.IsDisposed)
            {
                advancedAudio = new AdvancedAudio { Track = this.audioList.SelectedRows[0].DataBoundItem as AudioTrack };
            }

            advancedAudio.Show();
        }
    }
}