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
            this.audioList_moveup = new System.Windows.Forms.ToolStripMenuItem();
            this.audioList_movedown = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.audioList_remove = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_RemoveAudioTrack = new System.Windows.Forms.Button();
            this.btn_addAudioTrack = new System.Windows.Forms.Button();
            this.lbl_drc = new System.Windows.Forms.Label();
            this.lbl_drcHeader = new System.Windows.Forms.Label();
            this.tb_drc = new System.Windows.Forms.TrackBar();
            this.label68 = new System.Windows.Forms.Label();
            this.lbl_sampleRate = new System.Windows.Forms.Label();
            this.lbl_bitrate = new System.Windows.Forms.Label();
            this.label65 = new System.Windows.Forms.Label();
            this.lbl_mixdown = new System.Windows.Forms.Label();
            this.drp_audioEncoder = new System.Windows.Forms.ComboBox();
            this.label27 = new System.Windows.Forms.Label();
            this.drp_audioMix = new System.Windows.Forms.ComboBox();
            this.drp_audioTrack = new System.Windows.Forms.ComboBox();
            this.drp_audioBitrate = new System.Windows.Forms.ComboBox();
            this.drp_audioSample = new System.Windows.Forms.ComboBox();
            this.AudioTrackGroup = new System.Windows.Forms.GroupBox();
            this.AudioMenuRowHeightHack = new System.Windows.Forms.ImageList(this.components);
            this.audioList = new System.Windows.Forms.DataGridView();
            this.Track = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Source = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.AudioCodec = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Mixdown = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Samplerate = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Bitrate = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.DRC = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.audioMenu.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.tb_drc)).BeginInit();
            this.AudioTrackGroup.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.audioList)).BeginInit();
            this.SuspendLayout();
            // 
            // audioMenu
            // 
            this.audioMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.audioList_moveup,
            this.audioList_movedown,
            this.toolStripSeparator2,
            this.audioList_remove});
            this.audioMenu.Name = "audioMenu";
            this.audioMenu.Size = new System.Drawing.Size(139, 76);
            // 
            // audioList_moveup
            // 
            this.audioList_moveup.Name = "audioList_moveup";
            this.audioList_moveup.Size = new System.Drawing.Size(138, 22);
            this.audioList_moveup.Text = "Move Up";
            this.audioList_moveup.Click += new System.EventHandler(this.audioList_moveup_Click);
            // 
            // audioList_movedown
            // 
            this.audioList_movedown.Name = "audioList_movedown";
            this.audioList_movedown.Size = new System.Drawing.Size(138, 22);
            this.audioList_movedown.Text = "Move Down";
            this.audioList_movedown.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.audioList_movedown.Click += new System.EventHandler(this.audioList_movedown_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(135, 6);
            // 
            // audioList_remove
            // 
            this.audioList_remove.Name = "audioList_remove";
            this.audioList_remove.Size = new System.Drawing.Size(138, 22);
            this.audioList_remove.Text = "Remove";
            this.audioList_remove.Click += new System.EventHandler(this.audioList_remove_Click);
            // 
            // btn_RemoveAudioTrack
            // 
            this.btn_RemoveAudioTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_RemoveAudioTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_RemoveAudioTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_RemoveAudioTrack.Location = new System.Drawing.Point(99, 38);
            this.btn_RemoveAudioTrack.Name = "btn_RemoveAudioTrack";
            this.btn_RemoveAudioTrack.Size = new System.Drawing.Size(71, 23);
            this.btn_RemoveAudioTrack.TabIndex = 64;
            this.btn_RemoveAudioTrack.Text = "Remove";
            this.btn_RemoveAudioTrack.UseVisualStyleBackColor = false;
            this.btn_RemoveAudioTrack.Click += new System.EventHandler(this.btn_RemoveAudioTrack_Click);
            // 
            // btn_addAudioTrack
            // 
            this.btn_addAudioTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_addAudioTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_addAudioTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_addAudioTrack.Location = new System.Drawing.Point(16, 38);
            this.btn_addAudioTrack.Name = "btn_addAudioTrack";
            this.btn_addAudioTrack.Size = new System.Drawing.Size(77, 23);
            this.btn_addAudioTrack.TabIndex = 63;
            this.btn_addAudioTrack.Text = "Add Track";
            this.btn_addAudioTrack.UseVisualStyleBackColor = false;
            this.btn_addAudioTrack.Click += new System.EventHandler(this.btn_addAudioTrack_Click);
            // 
            // lbl_drc
            // 
            this.lbl_drc.AutoSize = true;
            this.lbl_drc.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drc.Location = new System.Drawing.Point(658, 37);
            this.lbl_drc.Name = "lbl_drc";
            this.lbl_drc.Size = new System.Drawing.Size(13, 13);
            this.lbl_drc.TabIndex = 62;
            this.lbl_drc.Text = "0";
            // 
            // lbl_drcHeader
            // 
            this.lbl_drcHeader.AutoSize = true;
            this.lbl_drcHeader.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drcHeader.Location = new System.Drawing.Point(607, 17);
            this.lbl_drcHeader.Name = "lbl_drcHeader";
            this.lbl_drcHeader.Size = new System.Drawing.Size(28, 13);
            this.lbl_drcHeader.TabIndex = 61;
            this.lbl_drcHeader.Text = "DRC";
            // 
            // tb_drc
            // 
            this.tb_drc.BackColor = System.Drawing.SystemColors.Window;
            this.tb_drc.LargeChange = 0;
            this.tb_drc.Location = new System.Drawing.Point(598, 29);
            this.tb_drc.Margin = new System.Windows.Forms.Padding(0);
            this.tb_drc.Maximum = 31;
            this.tb_drc.Name = "tb_drc";
            this.tb_drc.Size = new System.Drawing.Size(57, 45);
            this.tb_drc.TabIndex = 60;
            this.tb_drc.TickFrequency = 10;
            this.tb_drc.Scroll += new System.EventHandler(this.ControlChanged);
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
            // lbl_sampleRate
            // 
            this.lbl_sampleRate.AutoSize = true;
            this.lbl_sampleRate.BackColor = System.Drawing.Color.Transparent;
            this.lbl_sampleRate.Location = new System.Drawing.Point(463, 17);
            this.lbl_sampleRate.Name = "lbl_sampleRate";
            this.lbl_sampleRate.Size = new System.Drawing.Size(61, 13);
            this.lbl_sampleRate.TabIndex = 57;
            this.lbl_sampleRate.Text = "Samplerate";
            // 
            // lbl_bitrate
            // 
            this.lbl_bitrate.AutoSize = true;
            this.lbl_bitrate.BackColor = System.Drawing.Color.Transparent;
            this.lbl_bitrate.Location = new System.Drawing.Point(538, 17);
            this.lbl_bitrate.Name = "lbl_bitrate";
            this.lbl_bitrate.Size = new System.Drawing.Size(39, 13);
            this.lbl_bitrate.TabIndex = 59;
            this.lbl_bitrate.Text = "Bitrate";
            // 
            // label65
            // 
            this.label65.AutoSize = true;
            this.label65.BackColor = System.Drawing.Color.Transparent;
            this.label65.Location = new System.Drawing.Point(234, 17);
            this.label65.Name = "label65";
            this.label65.Size = new System.Drawing.Size(67, 13);
            this.label65.TabIndex = 53;
            this.label65.Text = "Audio Codec";
            // 
            // lbl_mixdown
            // 
            this.lbl_mixdown.AutoSize = true;
            this.lbl_mixdown.BackColor = System.Drawing.Color.Transparent;
            this.lbl_mixdown.Location = new System.Drawing.Point(368, 17);
            this.lbl_mixdown.Name = "lbl_mixdown";
            this.lbl_mixdown.Size = new System.Drawing.Size(49, 13);
            this.lbl_mixdown.TabIndex = 55;
            this.lbl_mixdown.Text = "Mixdown";
            // 
            // drp_audioEncoder
            // 
            this.drp_audioEncoder.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_audioEncoder.FormattingEnabled = true;
            this.drp_audioEncoder.Items.AddRange(new object[] {
            "AAC (faac)",
            "MP3 (lame)",
            "Vorbis (vorbis)",
            "AC3 Passthru",
            "AC3 (ffmpeg)",
            "DTS Passthru"});
            this.drp_audioEncoder.Location = new System.Drawing.Point(215, 34);
            this.drp_audioEncoder.Name = "drp_audioEncoder";
            this.drp_audioEncoder.Size = new System.Drawing.Size(111, 21);
            this.drp_audioEncoder.TabIndex = 52;
            this.drp_audioEncoder.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // label27
            // 
            this.label27.AutoSize = true;
            this.label27.BackColor = System.Drawing.Color.Transparent;
            this.label27.Location = new System.Drawing.Point(86, 17);
            this.label27.Name = "label27";
            this.label27.Size = new System.Drawing.Size(40, 13);
            this.label27.TabIndex = 51;
            this.label27.Text = "Source";
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
            this.drp_audioMix.Location = new System.Drawing.Point(332, 34);
            this.drp_audioMix.Name = "drp_audioMix";
            this.drp_audioMix.Size = new System.Drawing.Size(129, 21);
            this.drp_audioMix.TabIndex = 54;
            this.drp_audioMix.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // drp_audioTrack
            // 
            this.drp_audioTrack.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_audioTrack.FormattingEnabled = true;
            this.drp_audioTrack.Items.AddRange(new object[] {
            "Automatic"});
            this.drp_audioTrack.Location = new System.Drawing.Point(15, 34);
            this.drp_audioTrack.Name = "drp_audioTrack";
            this.drp_audioTrack.Size = new System.Drawing.Size(194, 21);
            this.drp_audioTrack.TabIndex = 50;
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
            this.drp_audioBitrate.Location = new System.Drawing.Point(525, 34);
            this.drp_audioBitrate.Name = "drp_audioBitrate";
            this.drp_audioBitrate.Size = new System.Drawing.Size(70, 21);
            this.drp_audioBitrate.TabIndex = 58;
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
            this.drp_audioSample.Location = new System.Drawing.Point(467, 34);
            this.drp_audioSample.Name = "drp_audioSample";
            this.drp_audioSample.Size = new System.Drawing.Size(55, 21);
            this.drp_audioSample.TabIndex = 56;
            this.drp_audioSample.SelectedIndexChanged += new System.EventHandler(this.ControlChanged);
            // 
            // AudioTrackGroup
            // 
            this.AudioTrackGroup.BackColor = System.Drawing.Color.Transparent;
            this.AudioTrackGroup.Controls.Add(this.drp_audioTrack);
            this.AudioTrackGroup.Controls.Add(this.drp_audioSample);
            this.AudioTrackGroup.Controls.Add(this.drp_audioBitrate);
            this.AudioTrackGroup.Controls.Add(this.drp_audioMix);
            this.AudioTrackGroup.Controls.Add(this.lbl_drc);
            this.AudioTrackGroup.Controls.Add(this.label27);
            this.AudioTrackGroup.Controls.Add(this.lbl_drcHeader);
            this.AudioTrackGroup.Controls.Add(this.drp_audioEncoder);
            this.AudioTrackGroup.Controls.Add(this.tb_drc);
            this.AudioTrackGroup.Controls.Add(this.lbl_mixdown);
            this.AudioTrackGroup.Controls.Add(this.label65);
            this.AudioTrackGroup.Controls.Add(this.lbl_sampleRate);
            this.AudioTrackGroup.Controls.Add(this.lbl_bitrate);
            this.AudioTrackGroup.Location = new System.Drawing.Point(16, 67);
            this.AudioTrackGroup.Name = "AudioTrackGroup";
            this.AudioTrackGroup.Size = new System.Drawing.Size(685, 77);
            this.AudioTrackGroup.TabIndex = 66;
            this.AudioTrackGroup.TabStop = false;
            this.AudioTrackGroup.Text = "Selected Track: New Track";
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
            this.audioList.BackgroundColor = System.Drawing.Color.White;
            this.audioList.ColumnHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.audioList.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.DisableResizing;
            this.audioList.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.Track,
            this.Source,
            this.AudioCodec,
            this.Mixdown,
            this.Samplerate,
            this.Bitrate,
            this.DRC});
            this.audioList.ContextMenuStrip = this.audioMenu;
            this.audioList.GridColor = System.Drawing.Color.White;
            this.audioList.Location = new System.Drawing.Point(16, 150);
            this.audioList.Name = "audioList";
            this.audioList.RowHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.audioList.RowHeadersVisible = false;
            this.audioList.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.AutoSizeToAllHeaders;
            this.audioList.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.audioList.ShowCellErrors = false;
            this.audioList.ShowCellToolTips = false;
            this.audioList.ShowEditingIcon = false;
            this.audioList.ShowRowErrors = false;
            this.audioList.Size = new System.Drawing.Size(685, 140);
            this.audioList.TabIndex = 67;
            this.audioList.SelectionChanged += new System.EventHandler(this.audioList_SelectionChanged);
            // 
            // Track
            // 
            this.Track.FillWeight = 304.2808F;
            this.Track.HeaderText = "Track";
            this.Track.Name = "Track";
            this.Track.ReadOnly = true;
            this.Track.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Track.Width = 50;
            // 
            // Source
            // 
            this.Source.FillWeight = 49.69727F;
            this.Source.HeaderText = "Source";
            this.Source.Name = "Source";
            this.Source.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Source.Width = 150;
            // 
            // AudioCodec
            // 
            this.AudioCodec.HeaderText = "Audio Codec";
            this.AudioCodec.Name = "AudioCodec";
            this.AudioCodec.ReadOnly = true;
            this.AudioCodec.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // Mixdown
            // 
            this.Mixdown.FillWeight = 49.69727F;
            this.Mixdown.HeaderText = "Mixdown";
            this.Mixdown.Name = "Mixdown";
            this.Mixdown.ReadOnly = true;
            this.Mixdown.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Mixdown.Width = 150;
            // 
            // Samplerate
            // 
            this.Samplerate.FillWeight = 49.69727F;
            this.Samplerate.HeaderText = "Samplerate";
            this.Samplerate.Name = "Samplerate";
            this.Samplerate.ReadOnly = true;
            this.Samplerate.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Samplerate.Width = 75;
            // 
            // Bitrate
            // 
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
            this.DRC.FillWeight = 96.36334F;
            this.DRC.HeaderText = "DRC";
            this.DRC.Name = "DRC";
            this.DRC.ReadOnly = true;
            this.DRC.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // AudioPanel
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.audioList);
            this.Controls.Add(this.label68);
            this.Controls.Add(this.btn_addAudioTrack);
            this.Controls.Add(this.AudioTrackGroup);
            this.Controls.Add(this.btn_RemoveAudioTrack);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "AudioPanel";
            this.Size = new System.Drawing.Size(720, 310);
            this.audioMenu.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.tb_drc)).EndInit();
            this.AudioTrackGroup.ResumeLayout(false);
            this.AudioTrackGroup.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.audioList)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_RemoveAudioTrack;
        private System.Windows.Forms.Button btn_addAudioTrack;
        internal System.Windows.Forms.Label lbl_drc;
        internal System.Windows.Forms.Label lbl_drcHeader;
        internal System.Windows.Forms.TrackBar tb_drc;
        internal System.Windows.Forms.Label label68;
        internal System.Windows.Forms.Label lbl_sampleRate;
        internal System.Windows.Forms.Label lbl_bitrate;
        internal System.Windows.Forms.Label label65;
        internal System.Windows.Forms.Label lbl_mixdown;
        internal System.Windows.Forms.ComboBox drp_audioEncoder;
        internal System.Windows.Forms.Label label27;
        internal System.Windows.Forms.ComboBox drp_audioMix;
        internal System.Windows.Forms.ComboBox drp_audioTrack;
        internal System.Windows.Forms.ComboBox drp_audioSample;
        private System.Windows.Forms.GroupBox AudioTrackGroup;
        private System.Windows.Forms.ImageList AudioMenuRowHeightHack;
        private System.Windows.Forms.ContextMenuStrip audioMenu;
        private System.Windows.Forms.ToolStripMenuItem audioList_moveup;
        private System.Windows.Forms.ToolStripMenuItem audioList_movedown;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem audioList_remove;
        internal System.Windows.Forms.ComboBox drp_audioBitrate;
        private System.Windows.Forms.DataGridView audioList;
        private System.Windows.Forms.DataGridViewTextBoxColumn Track;
        private System.Windows.Forms.DataGridViewTextBoxColumn Source;
        private System.Windows.Forms.DataGridViewTextBoxColumn AudioCodec;
        private System.Windows.Forms.DataGridViewTextBoxColumn Mixdown;
        private System.Windows.Forms.DataGridViewTextBoxColumn Samplerate;
        private System.Windows.Forms.DataGridViewTextBoxColumn Bitrate;
        private System.Windows.Forms.DataGridViewTextBoxColumn DRC;
    }
}
