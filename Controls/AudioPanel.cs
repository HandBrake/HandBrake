/*  AudioPanel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.Drawing;
    using System.Linq;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices;
    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;

    using Handbrake.ToolWindows;

    /// <summary>
    /// The AudioPanel Control
    /// </summary>
    public partial class AudioPanel : UserControl
    {
        #region Private Variables
        private readonly BindingList<AudioTrack> audioTracks = new BindingList<AudioTrack>();
        private const string Passthru = "Passthru";
        private AdvancedAudio advancedAudio = new AdvancedAudio();
        /// <summary>
        /// The User Setting Service.
        /// </summary>
        private readonly IUserSettingService UserSettingService = ServiceManager.UserSettingService;
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
                    AudioHelper.NoneFound 
                };

            this.audioList.AutoGenerateColumns = false;
            this.audioList.DataSource = audioTracks;

            drp_audioMix.SelectedItem = "Dolby Pro Logic II";
            drp_audioSample.SelectedIndex = 1;
            drp_audioBitrate.SelectedItem = "160";
            drp_audioEncoder.SelectedItem = "AAC (faac)";

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
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.Faac));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.ffaac));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.AacPassthru));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.Lame));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.Mp3Passthru));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.Ac3Passthrough));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.Ac3));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.DtsPassthrough));
            drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.DtsHDPassthrough));

            if (path.Contains("MKV"))
            {
                drp_audioEncoder.Items.Add(EnumHelper<AudioEncoder>.GetDisplay(AudioEncoder.Vorbis));
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
            return this.AudioTracks.Any(item => item.Encoder == AudioEncoder.Ac3Passthrough || item.Encoder == AudioEncoder.Ac3);
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
                if (track.Encoder == AudioEncoder.Ac3Passthrough || track.Encoder == AudioEncoder.DtsPassthrough ||
                    track.Encoder == AudioEncoder.DtsHDPassthrough || track.Encoder == AudioEncoder.AacPassthru || track.Encoder == AudioEncoder.Mp3Passthru)
                {
                    track.MixDown = HandBrake.Interop.Model.Encoding.Mixdown.Passthrough;
                    track.Bitrate = 0;
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
        public void SetTrackListAfterTitleChange(Title selectedTitle, Preset preset)
        {
            // Reset
            this.AudioTracks.Clear();
            this.ScannedTracks.Clear();

            if (selectedTitle.AudioTracks.Count == 0)
            {    
                this.ScannedTracks.Add(AudioHelper.NoneFound);
                this.drp_audioTrack.Refresh();
                drp_audioTrack.SelectedIndex = 0;
                return;
            }

            // Setup the Audio track source dropdown with the new audio tracks.
           // this.ScannedTracks.Clear();
            this.drp_audioTrack.SelectedItem = null;
            this.ScannedTracks = new BindingList<Audio>(selectedTitle.AudioTracks.ToList());
            drp_audioTrack.DataSource = this.ScannedTracks;

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

            if (selectedTitle.AudioTracks.Count > 0)
            {
                this.AutomaticTrackSelection();
            }
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

                        // Correct bad passthru option
                        if (this.IsIncompatiblePassthru(track))
                        {
                            AudioEncoder encoder = GetCompatiblePassthru(track);
                            drp_audioEncoder.SelectedItem = EnumHelper<AudioEncoder>.GetDisplay(encoder);
                        } 
                    }
                    break;
                case "drp_audioEncoder":
                    SetMixDown(EnumHelper<Mixdown>.GetDisplay(track.MixDown));

                    // Configure the widgets with values
                    if (drp_audioEncoder.Text.Contains(Passthru))
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

                    // Correct bad passthru option
                    if (this.IsIncompatiblePassthru(track))
                    {
                        AudioEncoder encoder = GetCompatiblePassthru(track);
                        drp_audioEncoder.SelectedItem = EnumHelper<AudioEncoder>.GetDisplay(encoder);
                    }
                    break;
                case "drp_audioMix":
                    SetBitrate(track.Bitrate);

                    if (drp_audioMix.SelectedItem != null)
                    {
                        track.MixDown = EnumHelper<Mixdown>.GetValue(drp_audioMix.Text);
                    }

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
                    drp_audioEncoder.SelectedItem = EnumHelper<AudioEncoder>.GetDisplay(track.Encoder);
                    drp_audioMix.SelectedItem = EnumHelper<Mixdown>.GetDisplay(track.MixDown);
                    drp_audioSample.SelectedItem = track.SampleRateDisplayValue;
                    drp_audioBitrate.SelectedItem = track.BitRateDisplayValue;

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
        /// Add all the Audio Tracks that are not currently on the Lust
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The EventArgs</param>
        private void mnu_AddAll_Click(object sender, EventArgs e)
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

            bool trackAdded = false;
            foreach (Audio sourceTrack in this.ScannedTracks)
            {
                // Check if the Track already exists on the list. If it does, skip to the next
                bool foundTrack = false;
                foreach (AudioTrack currentTrack in this.AudioTracks)
                {
                    if (currentTrack.Track.HasValue && currentTrack.Track.Value == sourceTrack.TrackNumber)
                    {
                        // Set a flag to indicate we've found a track
                        foundTrack = true;
                        continue;
                    }

                    if (this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AddOnlyOneAudioPerLanguage) && currentTrack.TrackDisplay.Contains(sourceTrack.Language))
                    {
                        foundTrack = true;
                        continue;
                    }
                }

                if (foundTrack)
                {
                    // Skip to the nxet Source Track, We already have this one in the list.
                    continue;
                }

                // Create the Model
                AudioTrack track = new AudioTrack
                {
                    ScannedTrack = sourceTrack,
                };

                this.audioTracks.Add(track);
                trackAdded = true;
            }

            // If we added a track, then fire the event
            if (trackAdded)
            {
                // The Audio List has changed to raise the event.
                if (this.AudioListChanged != null)
                    this.AudioListChanged(this, new EventArgs());
            }
        }

        /// <summary>
        /// Remove an Audio Track
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The Event Args
        /// </param>
        private void Btn_remove_track_click(object sender, EventArgs e)
        {
            RemoveTrack();

            if (this.AudioTracks.Count == 0)
            {
                drp_audioMix.Enabled =
                    drp_audioBitrate.Enabled = drp_audioSample.Enabled = btn_AdvancedAudio.Enabled = true;
            }
        }

        /// <summary>
        /// Clear all audio tracks
        /// </summary>
        /// <param name="sender">
        /// The Sender
        /// </param>
        /// <param name="e">
        /// The Event Args
        /// </param>
        private void Mnu_clear_all_click(object sender, EventArgs e)
        {
            this.ClearAudioList();
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
            // Sanity check that we have Audio Tracks and if not, clear the track list.
            if (drp_audioTrack.SelectedItem != null && drp_audioTrack.SelectedItem.ToString() == AudioHelper.NoneFound.Description)
            {
                this.AudioTracks.Clear();
                return;
            }

            // Handle Preferred Language
            if (this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage) == "Any")
            {
                drp_audioTrack.SelectedIndex = 0;
                foreach (AudioTrack track in this.audioTracks)
                {
                    if (this.drp_audioTrack.SelectedItem != null)
                    {
                        track.ScannedTrack = this.drp_audioTrack.SelectedItem as Audio;
                    }
                }

                return;
            } 
            else
            {
                foreach (Audio item in drp_audioTrack.Items)
                {
                    if (item.Language.Contains(this.UserSettingService.GetUserSetting<string>(UserSettingConstants.NativeLanguage)))
                    {
                        drp_audioTrack.SelectedItem = item;
                        break;
                    }
                }
                
                foreach (AudioTrack track in this.audioTracks)
                {
                    if (this.drp_audioTrack.SelectedItem != null)
                    {
                        track.ScannedTrack = this.drp_audioTrack.SelectedItem as Audio;
                    }
                }
            }

            // Array with the Index numbers of the prefered and additional languages. 
            // This allows to have for each language the order in which they appear in the DVD list.
            Dictionary<String, ArrayList> languageIndex = new Dictionary<String, ArrayList>();

            // Now add any additional Langauges tracks on top of the presets tracks.
            int mode = this.UserSettingService.GetUserSetting<int>(UserSettingConstants.DubModeAudio);
            ArrayList languageOrder = new ArrayList();    // This is used to keep the Prefered Language in the front and the other languages in order. TODO this is no longer required, refactor this.
            if (mode > 0)
            {
                foreach (string item in this.UserSettingService.GetUserSetting<StringCollection>(UserSettingConstants.SelectedLanguages))
                {
                    if (!languageIndex.ContainsKey(item))
                    {
                        languageIndex.Add(item, new ArrayList());
                        languageOrder.Add(item);
                    }
                }

                bool elementFound = false;
                int i = 0;
                foreach (object item in drp_audioTrack.Items)
                {
                    foreach (KeyValuePair<String, ArrayList> kvp in languageIndex)
                    {
                        if (item.ToString().Contains(kvp.Key))
                        {
                            // Only the first Element if the "Only One Audio"-option is chosen.
                            if (!this.UserSettingService.GetUserSetting<bool>(UserSettingConstants.AddOnlyOneAudioPerLanguage) || kvp.Value.Count == 0)
                            {
                                kvp.Value.Add(i);
                            }

                            elementFound = true;
                        }
                    }

                    i++;
                }

                // There are no additional Languages, so we don't need to continue processing.
                if (!elementFound)
                {
                   // return;
                }
            }

            switch (mode)
            {
                case 1: // Adding all remaining audio tracks
                    this.mnu_AddAll_Click(this, EventArgs.Empty);
                    break;
                case 2: // Add Langauges tracks for the additional languages selected, in-order.
                    audioList.ClearSelection();
                    foreach (string item in languageOrder)
                    {
                        if (languageIndex[item].Count > 0)
                        {
                            foreach (int i in languageIndex[item])
                            {
                                drp_audioTrack.SelectedIndex = i;
                                if (drp_audioTrack.SelectedItem != null)
                                {
                                    Audio track = drp_audioTrack.SelectedItem as Audio;
                                    if (track != null)
                                    {
                                        if (!TrackExists(track))
                                        {
                                            this.AddAudioTrack_Click(this, EventArgs.Empty);
                                            audioList.ClearSelection();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
            }
        }

        /// <summary>
        /// Clear the audio list
        /// </summary>
        private void ClearAudioList()
        {
            this.AudioTracks.Clear();

            drp_audioMix.Enabled = drp_audioBitrate.Enabled = drp_audioSample.Enabled = btn_AdvancedAudio.Enabled = true;

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
        /// <param name="currentValue">
        /// The current Value.
        /// </param>
        private void SetBitrate(int currentValue)
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
                case "AAC (ffmpeg)":
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
            }

            if (drp_audioEncoder.Text.Contains(Passthru))
            {
                drp_audioBitrate.Items.Add("Auto");
                defaultRate = "Auto";
                drp_audioSample.SelectedItem = "Auto";
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

            // Set the Current Value, or default value if the value is out of bounds

            if (currentValue <= max && currentValue != 0)
            {
                drp_audioBitrate.SelectedItem = currentValue.ToString();
            }
            else
            {
                drp_audioBitrate.SelectedItem = defaultRate;
            }
        }

        /// <summary>
        /// Set the mixdown dropdown
        /// </summary>
        /// <param name="currentMixdown">
        /// The current Mixdown.
        /// </param>
        private void SetMixDown(string currentMixdown)
        {
            drp_audioMix.Items.Clear();
            drp_audioMix.Items.Add("Mono");
            drp_audioMix.Items.Add("Stereo");
            drp_audioMix.Items.Add("Dolby Surround");
            drp_audioMix.Items.Add("Dolby Pro Logic II");
            drp_audioMix.Items.Add("6 Channel Discrete");
            drp_audioMix.Items.Add(Passthru);

            switch (drp_audioEncoder.Text)
            {
                case "AAC (faac)":
                case "AAC (ffmpeg)":
                    drp_audioMix.Items.Remove(Passthru);
                    drp_audioMix.SelectedItem = currentMixdown ?? "Dolby Pro Logic II";
                    break;
                case "MP3 (lame)":
                    drp_audioMix.Items.Remove("6 Channel Discrete");
                    drp_audioMix.Items.Remove(Passthru);
                    drp_audioMix.SelectedItem = currentMixdown ?? "Dolby Pro Logic II";
                    break;
                case "Vorbis (vorbis)":
                    drp_audioMix.Items.Remove(Passthru);
                    drp_audioMix.SelectedItem = currentMixdown ?? "Dolby Pro Logic II";
                    break;
                case "AC3 (ffmpeg)":
                    drp_audioMix.Items.Remove(Passthru);
                    drp_audioMix.SelectedItem = currentMixdown ?? "Dolby Pro Logic II";
                    break;
                case "AC3 Passthru":
                case "DTS Passthru":
                case "DTS-HD Passthru":
                case "AAC Passthru":
                case "MP3 Passthru":
                    drp_audioMix.SelectedItem = Passthru;
                    break;
            }

            if (drp_audioMix.SelectedItem == null)
            {
                drp_audioMix.SelectedItem = "Dolby Pro Logic II";
            }
        }

        /// <summary>
        /// Check if a track already exists
        /// </summary>
        /// <param name="sourceTrack">
        /// The source track.
        /// </param>
        /// <returns>
        /// True if it does
        /// </returns>
        private bool TrackExists(Audio sourceTrack)
        {
            foreach (AudioTrack currentTrack in this.AudioTracks)
            {
                if (currentTrack.Track.HasValue && currentTrack.Track.Value == sourceTrack.TrackNumber)
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// For a given Audio Track, is the chosen Passthru option supported
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        /// <returns>
        /// True if it is.
        /// </returns>
        private bool IsIncompatiblePassthru(AudioTrack track)
        {
            if (track.ScannedTrack == null || string.IsNullOrEmpty(track.ScannedTrack.Format))
            {
                return false;
            }


            // If the track isn't AC3, and the encoder is, change it.
            if (track.Encoder == AudioEncoder.Ac3Passthrough && !track.ScannedTrack.Format.Contains("AC3"))
            {
                return true;
            }

            // If the track isn't DTS, and the encoder is, change it.
            if (track.Encoder == AudioEncoder.DtsPassthrough && !track.ScannedTrack.Format.Contains("DTS"))
            {
                return true;
            }

            // If the track isn't AAC, and the encoder is, change it.
            if (track.Encoder == AudioEncoder.AacPassthru && !track.ScannedTrack.Format.Contains("aac"))
            {
                return true;
            }

            // If the track isn't MP3, and the encoder is, change it.
            if (track.Encoder == AudioEncoder.Mp3Passthru && !track.ScannedTrack.Format.Contains("mp3"))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Get a compatible passthru, or default to aac.
        /// </summary>
        /// <param name="track">
        /// The track.
        /// </param>
        /// <returns>
        /// AN Audio encoder.
        /// </returns>
        private AudioEncoder GetCompatiblePassthru(AudioTrack track)
        {
            if (track.ScannedTrack.Format.Contains("AC3"))
            {
                return AudioEncoder.Ac3Passthrough;
            }

            if (track.ScannedTrack.Format.Contains("DTS"))
            {
                return AudioEncoder.DtsPassthrough;
            }

            if (track.ScannedTrack.Format.Contains("aac"))
            {
                return AudioEncoder.AacPassthru;
            }

            if (track.ScannedTrack.Format.Contains("mp3"))
            {
                return AudioEncoder.Mp3Passthru;
            }

            return AudioEncoder.Faac;
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