/*  AudioPanel.Designer.cs $
 	
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    partial class AudioPanel
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.audioMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.audioList_MoveToTop = new System.Windows.Forms.ToolStripMenuItem();
            this.audioList_moveup = new System.Windows.Forms.ToolStripMenuItem();
            this.audioList_movedown = new System.Windows.Forms.ToolStripMenuItem();
            this.audioList_MoveToBottom = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.audioList_remove = new System.Windows.Forms.ToolStripMenuItem();
            this.label68 = new System.Windows.Forms.Label();
            this.drp_audioEncoder = new System.Windows.Forms.ComboBox();
            this.drp_audioMix = new System.Windows.Forms.ComboBox();
            this.drp_audioTrack = new System.Windows.Forms.ComboBox();
            this.drp_audioBitrate = new System.Windows.Forms.ComboBox();
            this.drp_audioSample = new System.Windows.Forms.ComboBox();
            this.AudioMenuRowHeightHack = new System.Windows.Forms.ImageList(this.components);
            this.audioList = new System.Windows.Forms.DataGridView();
            this.Source = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.AudioCodec = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Mixdown = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Samplerate = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Bitrate = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.DRC = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Gain = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ToolTips = new System.Windows.Forms.ToolTip(this.components);
            this.btn_AdvancedAudio = new System.Windows.Forms.Button();
            this.btn_addAudioTrack = new wyDay.Controls.SplitButton();
            this.AddTrackMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_AddAll = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_RemoveTrack = new wyDay.Controls.SplitButton();
            this.RemoveTrackMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_ClearAll = new System.Windows.Forms.ToolStripMenuItem();
            this.audioMenu.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.audioList)).BeginInit();
            this.AddTrackMenu.SuspendLayout();
            this.RemoveTrackMenu.SuspendLayout();
            this.SuspendLayout();
            // 
            // audioMenu
            // 
            this.audioMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.audioList_MoveToTop,
            this.audioList_moveup,
            this.audioList_movedown,
            this.audioList_MoveToBottom,
            this.toolStripSeparator2,
            this.audioList_remove});
            this.audioMenu.Name = "audioMenu";
            this.audioMenu.Size = new System.Drawing.Size(165, 120);
            // 
            // audioList_MoveToTop
            // 
            this.audioList_MoveToTop.Name = "audioList_MoveToTop";
            this.audioList_MoveToTop.Size = new System.Drawing.Size(164, 22);
            this.audioList_MoveToTop.Text = "Move to Top";
            this.audioList_MoveToTop.Click += new System.EventHandler(this.audioList_MoveToTop_Click);
            // 
            // audioList_moveup
            // 
            this.audioList_moveup.Name = "audioList_moveup";
            this.audioList_moveup.Size = new System.Drawing.Size(164, 22);
            this.audioList_moveup.Text = "Move Up";
            this.audioList_moveup.Click += new System.EventHandler(this.AudioList_moveup_Click);
            // 
            // audioList_movedown
            // 
            this.audioList_movedown.Name = "audioList_movedown";
            this.audioList_movedown.Size = new System.Drawing.Size(164, 22);
            this.audioList_movedown.Text = "Move Down";
            this.audioList_movedown.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.audioList_movedown.Click += new System.EventHandler(this.AudioList_movedown_Click);
            // 
            // audioList_MoveToBottom
            // 
            this.audioList_MoveToBottom.Name = "audioList_MoveToBottom";
            this.audioList_MoveToBottom.Size = new System.Drawing.Size(164, 22);
            this.audioList_MoveToBottom.Text = "Move To Bottom";
            this.audioList_MoveToBottom.Click += new System.EventHandler(this.audioList_MoveToBottom_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(161, 6);
            // 
            // audioList_remove
            // 
            this.audioList_remove.Name = "audioList_remove";
            this.audioList_remove.Size = new System.Drawing.Size(164, 22);
            this.audioList_remove.Text = "Remove";
            this.audioList_remove.Click += new System.EventHandler(this.AudioList_remove_Click);
            // 
            // label68
            // 
            this.label68.AutoSize = true;
            this.label68.BackColor = System.Drawing.Color.Transparent;
            this.label68.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label68.Location = new System.Drawing.Point(13, 13);
            this.label68.Name = "label68";
            this.label68.Size = new System.Drawing.Size(80, 13);
            this.label68.TabIndex = 49;
            this.label68.Text = "Audio Tracks";
            // 
            // drp_audioEncoder
            // 
            this.drp_audioEncoder.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_audioEncoder.FormattingEnabled = true;
            this.drp_audioEncoder.Items.AddRange(new object[] {
            "AAC (faac)",
            "AAC (ffmpeg)",
            "MP3 (lame)",
            "Vorbis (vorbis)",
            "AC3 Passthru",
            "AC3 (ffmpeg)",
            "DTS Passthru"});
            this.drp_audioEncoder.Location = new System.Drawing.Point(191, 66);
            this.drp_audioEncoder.Name = "drp_audioEncoder";
            this.drp_audioEncoder.Size = new System.Drawing.Size(111, 21);
            this.drp_audioEncoder.TabIndex = 52;
            this.ToolTips.SetToolTip(this.drp_audioEncoder, "Set the audio codec to encode the selected track with.");
            this.drp_audioEncoder.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // drp_audioMix
            // 
            this.drp_audioMix.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_audioMix.FormattingEnabled = true;
            this.drp_audioMix.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audioMix.Location = new System.Drawing.Point(309, 66);
            this.drp_audioMix.Name = "drp_audioMix";
            this.drp_audioMix.Size = new System.Drawing.Size(147, 21);
            this.drp_audioMix.TabIndex = 54;
            this.ToolTips.SetToolTip(this.drp_audioMix, "Set the mixdown for the selected audio track.");
            this.drp_audioMix.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // drp_audioTrack
            // 
            this.drp_audioTrack.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_audioTrack.FormattingEnabled = true;
            this.drp_audioTrack.Items.AddRange(new object[] {
            "Automatic"});
            this.drp_audioTrack.Location = new System.Drawing.Point(16, 66);
            this.drp_audioTrack.Name = "drp_audioTrack";
            this.drp_audioTrack.Size = new System.Drawing.Size(170, 21);
            this.drp_audioTrack.TabIndex = 50;
            this.ToolTips.SetToolTip(this.drp_audioTrack, "The list of audio tracks available from your source.");
            this.drp_audioTrack.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // drp_audioBitrate
            // 
            this.drp_audioBitrate.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_audioBitrate.FormattingEnabled = true;
            this.drp_audioBitrate.Items.AddRange(new object[] {
            "32",
            "40",
            "48",
            "56",
            "64",
            "80",
            "96",
            "112",
            "128",
            "160"});
            this.drp_audioBitrate.Location = new System.Drawing.Point(534, 66);
            this.drp_audioBitrate.Name = "drp_audioBitrate";
            this.drp_audioBitrate.Size = new System.Drawing.Size(67, 21);
            this.drp_audioBitrate.TabIndex = 58;
            this.ToolTips.SetToolTip(this.drp_audioBitrate, "Set the bitrate for the selected audio track.");
            this.drp_audioBitrate.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // drp_audioSample
            // 
            this.drp_audioSample.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_audioSample.FormattingEnabled = true;
            this.drp_audioSample.Items.AddRange(new object[] {
            "Auto",
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audioSample.Location = new System.Drawing.Point(461, 66);
            this.drp_audioSample.Name = "drp_audioSample";
            this.drp_audioSample.Size = new System.Drawing.Size(67, 21);
            this.drp_audioSample.TabIndex = 56;
            this.ToolTips.SetToolTip(this.drp_audioSample, "Set the samplerate for the selected audio track.");
            this.drp_audioSample.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // AudioMenuRowHeightHack
            // 
            this.AudioMenuRowHeightHack.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
            this.AudioMenuRowHeightHack.ImageSize = new System.Drawing.Size(1, 18);
            this.AudioMenuRowHeightHack.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // audioList
            // 
            this.audioList.AllowUserToAddRows = false;
            this.audioList.AllowUserToResizeColumns = false;
            this.audioList.AllowUserToResizeRows = false;
            this.audioList.BackgroundColor = System.Drawing.Color.White;
            this.audioList.ColumnHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.audioList.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.DisableResizing;
            this.audioList.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.Source,
            this.AudioCodec,
            this.Mixdown,
            this.Samplerate,
            this.Bitrate,
            this.DRC,
            this.Gain});
            this.audioList.ContextMenuStrip = this.audioMenu;
            this.audioList.GridColor = System.Drawing.Color.White;
            this.audioList.Location = new System.Drawing.Point(16, 94);
            this.audioList.Name = "audioList";
            this.audioList.RowHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.audioList.RowHeadersVisible = false;
            this.audioList.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.AutoSizeToAllHeaders;
            this.audioList.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.audioList.ShowCellErrors = false;
            this.audioList.ShowCellToolTips = false;
            this.audioList.ShowEditingIcon = false;
            this.audioList.ShowRowErrors = false;
            this.audioList.Size = new System.Drawing.Size(685, 200);
            this.audioList.TabIndex = 67;
            this.ToolTips.SetToolTip(this.audioList, "The audio tracks to be encoded into the output file.");
            this.audioList.SelectionChanged += new System.EventHandler(this.audioList_SelectionChanged);
            // 
            // Source
            // 
            this.Source.DataPropertyName = "TrackDisplay";
            this.Source.FillWeight = 49.69727F;
            this.Source.HeaderText = "Source";
            this.Source.Name = "Source";
            this.Source.ReadOnly = true;
            this.Source.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Source.Width = 170;
            // 
            // AudioCodec
            // 
            this.AudioCodec.DataPropertyName = "Encoder";
            this.AudioCodec.HeaderText = "Audio Codec";
            this.AudioCodec.Name = "AudioCodec";
            this.AudioCodec.ReadOnly = true;
            this.AudioCodec.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.AudioCodec.Width = 120;
            // 
            // Mixdown
            // 
            this.Mixdown.DataPropertyName = "Mixdown";
            this.Mixdown.FillWeight = 49.69727F;
            this.Mixdown.HeaderText = "Mixdown";
            this.Mixdown.Name = "Mixdown";
            this.Mixdown.ReadOnly = true;
            this.Mixdown.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Mixdown.Width = 150;
            // 
            // Samplerate
            // 
            this.Samplerate.DataPropertyName = "SampleRateDisplayValue";
            this.Samplerate.FillWeight = 49.69727F;
            this.Samplerate.HeaderText = "Samplerate";
            this.Samplerate.Name = "Samplerate";
            this.Samplerate.ReadOnly = true;
            this.Samplerate.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Samplerate.Width = 75;
            // 
            // Bitrate
            // 
            this.Bitrate.DataPropertyName = "BitRateDisplayValue";
            this.Bitrate.FillWeight = 49.69727F;
            this.Bitrate.HeaderText = "Bitrate";
            this.Bitrate.Name = "Bitrate";
            this.Bitrate.ReadOnly = true;
            this.Bitrate.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Bitrate.Width = 75;
            // 
            // DRC
            // 
            this.DRC.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.DRC.DataPropertyName = "DRC";
            this.DRC.FillWeight = 96.36334F;
            this.DRC.HeaderText = "DRC";
            this.DRC.Name = "DRC";
            this.DRC.ReadOnly = true;
            this.DRC.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.DRC.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // Gain
            // 
            this.Gain.DataPropertyName = "Gain";
            this.Gain.HeaderText = "Gain (dB)";
            this.Gain.Name = "Gain";
            this.Gain.ReadOnly = true;
            this.Gain.Width = 60;
            // 
            // btn_AdvancedAudio
            // 
            this.btn_AdvancedAudio.BackColor = System.Drawing.Color.Transparent;
            this.btn_AdvancedAudio.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_AdvancedAudio.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_AdvancedAudio.Location = new System.Drawing.Point(607, 65);
            this.btn_AdvancedAudio.Name = "btn_AdvancedAudio";
            this.btn_AdvancedAudio.Size = new System.Drawing.Size(94, 23);
            this.btn_AdvancedAudio.TabIndex = 70;
            this.btn_AdvancedAudio.Text = "Advanced";
            this.btn_AdvancedAudio.UseVisualStyleBackColor = false;
            this.btn_AdvancedAudio.Click += new System.EventHandler(this.btn_AdvancedAudio_Click);
            // 
            // btn_addAudioTrack
            // 
            this.btn_addAudioTrack.AutoSize = true;
            this.btn_addAudioTrack.ContextMenuStrip = this.AddTrackMenu;
            this.btn_addAudioTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_addAudioTrack.ForeColor = System.Drawing.Color.DarkOrange;
            this.btn_addAudioTrack.Location = new System.Drawing.Point(16, 37);
            this.btn_addAudioTrack.Name = "btn_addAudioTrack";
            this.btn_addAudioTrack.Size = new System.Drawing.Size(92, 23);
            this.btn_addAudioTrack.SplitMenuStrip = this.AddTrackMenu;
            this.btn_addAudioTrack.TabIndex = 72;
            this.btn_addAudioTrack.Text = "Add Track";
            this.btn_addAudioTrack.UseVisualStyleBackColor = true;
            this.btn_addAudioTrack.Click += new System.EventHandler(this.AddAudioTrack_Click);
            // 
            // AddTrackMenu
            // 
            this.AddTrackMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_AddAll});
            this.AddTrackMenu.Name = "audioMenu";
            this.AddTrackMenu.Size = new System.Drawing.Size(114, 26);
            // 
            // mnu_AddAll
            // 
            this.mnu_AddAll.Name = "mnu_AddAll";
            this.mnu_AddAll.Size = new System.Drawing.Size(113, 22);
            this.mnu_AddAll.Text = "Add All";
            this.mnu_AddAll.Click += new System.EventHandler(this.mnu_AddAll_Click);
            // 
            // btn_RemoveTrack
            // 
            this.btn_RemoveTrack.AutoSize = true;
            this.btn_RemoveTrack.ContextMenuStrip = this.RemoveTrackMenu;
            this.btn_RemoveTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_RemoveTrack.ForeColor = System.Drawing.Color.DarkOrange;
            this.btn_RemoveTrack.Location = new System.Drawing.Point(114, 37);
            this.btn_RemoveTrack.Name = "btn_RemoveTrack";
            this.btn_RemoveTrack.Size = new System.Drawing.Size(82, 23);
            this.btn_RemoveTrack.SplitMenuStrip = this.RemoveTrackMenu;
            this.btn_RemoveTrack.TabIndex = 73;
            this.btn_RemoveTrack.Text = "Remove";
            this.btn_RemoveTrack.UseVisualStyleBackColor = true;
            this.btn_RemoveTrack.Click += new System.EventHandler(this.Btn_remove_track_click);
            // 
            // RemoveTrackMenu
            // 
            this.RemoveTrackMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_ClearAll});
            this.RemoveTrackMenu.Name = "audioMenu";
            this.RemoveTrackMenu.Size = new System.Drawing.Size(119, 26);
            // 
            // mnu_ClearAll
            // 
            this.mnu_ClearAll.Name = "mnu_ClearAll";
            this.mnu_ClearAll.Size = new System.Drawing.Size(152, 22);
            this.mnu_ClearAll.Text = "Clear All";
            this.mnu_ClearAll.Click += new System.EventHandler(this.Mnu_clear_all_click);
            // 
            // AudioPanel
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.drp_audioTrack);
            this.Controls.Add(this.btn_AdvancedAudio);
            this.Controls.Add(this.drp_audioSample);
            this.Controls.Add(this.btn_RemoveTrack);
            this.Controls.Add(this.audioList);
            this.Controls.Add(this.label68);
            this.Controls.Add(this.btn_addAudioTrack);
            this.Controls.Add(this.drp_audioBitrate);
            this.Controls.Add(this.drp_audioMix);
            this.Controls.Add(this.drp_audioEncoder);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "AudioPanel";
            this.Size = new System.Drawing.Size(720, 310);
            this.audioMenu.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.audioList)).EndInit();
            this.AddTrackMenu.ResumeLayout(false);
            this.RemoveTrackMenu.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label label68;
        internal System.Windows.Forms.ComboBox drp_audioEncoder;
        internal System.Windows.Forms.ComboBox drp_audioMix;
        internal System.Windows.Forms.ComboBox drp_audioTrack;
        internal System.Windows.Forms.ComboBox drp_audioSample;
        private System.Windows.Forms.ImageList AudioMenuRowHeightHack;
        private System.Windows.Forms.ContextMenuStrip audioMenu;
        private System.Windows.Forms.ToolStripMenuItem audioList_moveup;
        private System.Windows.Forms.ToolStripMenuItem audioList_movedown;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem audioList_remove;
        internal System.Windows.Forms.ComboBox drp_audioBitrate;
        private System.Windows.Forms.DataGridView audioList;
        private System.Windows.Forms.ToolTip ToolTips;
        private System.Windows.Forms.Button btn_AdvancedAudio;
        private System.Windows.Forms.DataGridViewTextBoxColumn Source;
        private System.Windows.Forms.DataGridViewTextBoxColumn AudioCodec;
        private System.Windows.Forms.DataGridViewTextBoxColumn Mixdown;
        private System.Windows.Forms.DataGridViewTextBoxColumn Samplerate;
        private System.Windows.Forms.DataGridViewTextBoxColumn Bitrate;
        private System.Windows.Forms.DataGridViewTextBoxColumn DRC;
        private System.Windows.Forms.DataGridViewTextBoxColumn Gain;
        private System.Windows.Forms.ToolStripMenuItem audioList_MoveToTop;
        private System.Windows.Forms.ToolStripMenuItem audioList_MoveToBottom;
        private wyDay.Controls.SplitButton btn_addAudioTrack;
        private System.Windows.Forms.ContextMenuStrip AddTrackMenu;
        private System.Windows.Forms.ToolStripMenuItem mnu_AddAll;
        private wyDay.Controls.SplitButton btn_RemoveTrack;
        private System.Windows.Forms.ContextMenuStrip RemoveTrackMenu;
        private System.Windows.Forms.ToolStripMenuItem mnu_ClearAll;
    }
}
