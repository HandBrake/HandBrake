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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AudioPanel));
            this.audioMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.audioList_moveup = new System.Windows.Forms.ToolStripMenuItem();
            this.audioList_movedown = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.audioList_remove = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_RemoveAudioTrack = new System.Windows.Forms.Button();
            this.btn_addAudioTrack = new System.Windows.Forms.Button();
            this.lbl_drc = new System.Windows.Forms.Label();
            this.tb_drc = new System.Windows.Forms.TrackBar();
            this.label68 = new System.Windows.Forms.Label();
            this.drp_audioEncoder = new System.Windows.Forms.ComboBox();
            this.drp_audioMix = new System.Windows.Forms.ComboBox();
            this.drp_audioTrack = new System.Windows.Forms.ComboBox();
            this.drp_audioBitrate = new System.Windows.Forms.ComboBox();
            this.drp_audioSample = new System.Windows.Forms.ComboBox();
            this.AudioMenuRowHeightHack = new System.Windows.Forms.ImageList(this.components);
            this.label1 = new System.Windows.Forms.Label();
            this.lbl_audioTrack = new System.Windows.Forms.Label();
            this.DRC = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Bitrate = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Samplerate = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Mixdown = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.AudioCodec = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Source = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.audioList = new System.Windows.Forms.DataGridView();
            this.ToolTips = new System.Windows.Forms.ToolTip(this.components);
            this.audioMenu.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.tb_drc)).BeginInit();
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
            this.audioList_moveup.Click += new System.EventHandler(this.AudioList_moveup_Click);
            // 
            // audioList_movedown
            // 
            this.audioList_movedown.Name = "audioList_movedown";
            this.audioList_movedown.Size = new System.Drawing.Size(138, 22);
            this.audioList_movedown.Text = "Move Down";
            this.audioList_movedown.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.audioList_movedown.Click += new System.EventHandler(this.AudioList_movedown_Click);
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
            this.audioList_remove.Click += new System.EventHandler(this.AudioList_remove_Click);
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
            this.btn_RemoveAudioTrack.Click += new System.EventHandler(this.RemoveAudioTrack_Click);
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
            this.btn_addAudioTrack.Click += new System.EventHandler(this.AddAudioTrack_Click);
            // 
            // lbl_drc
            // 
            this.lbl_drc.AutoSize = true;
            this.lbl_drc.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drc.Location = new System.Drawing.Point(666, 90);
            this.lbl_drc.Name = "lbl_drc";
            this.lbl_drc.Size = new System.Drawing.Size(13, 13);
            this.lbl_drc.TabIndex = 62;
            this.lbl_drc.Text = "0";
            // 
            // tb_drc
            // 
            this.tb_drc.BackColor = System.Drawing.SystemColors.Window;
            this.tb_drc.LargeChange = 0;
            this.tb_drc.Location = new System.Drawing.Point(608, 84);
            this.tb_drc.Margin = new System.Windows.Forms.Padding(0);
            this.tb_drc.Maximum = 31;
            this.tb_drc.Name = "tb_drc";
            this.tb_drc.Size = new System.Drawing.Size(57, 45);
            this.tb_drc.TabIndex = 60;
            this.tb_drc.TickFrequency = 10;
            this.ToolTips.SetToolTip(this.tb_drc, resources.GetString("tb_drc.ToolTip"));
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
            this.drp_audioEncoder.Location = new System.Drawing.Point(191, 87);
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
            this.drp_audioMix.Location = new System.Drawing.Point(309, 87);
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
            this.drp_audioTrack.Location = new System.Drawing.Point(16, 87);
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
            this.drp_audioBitrate.Location = new System.Drawing.Point(534, 87);
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
            this.drp_audioSample.Location = new System.Drawing.Point(461, 87);
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
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(13, 66);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(94, 13);
            this.label1.TabIndex = 68;
            this.label1.Text = "Selected Track:";
            // 
            // lbl_audioTrack
            // 
            this.lbl_audioTrack.AutoSize = true;
            this.lbl_audioTrack.BackColor = System.Drawing.Color.Transparent;
            this.lbl_audioTrack.Location = new System.Drawing.Point(113, 66);
            this.lbl_audioTrack.Name = "lbl_audioTrack";
            this.lbl_audioTrack.Size = new System.Drawing.Size(32, 13);
            this.lbl_audioTrack.TabIndex = 69;
            this.lbl_audioTrack.Text = "None";
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
            // AudioCodec
            // 
            this.AudioCodec.DataPropertyName = "Encoder";
            this.AudioCodec.HeaderText = "Audio Codec";
            this.AudioCodec.Name = "AudioCodec";
            this.AudioCodec.ReadOnly = true;
            this.AudioCodec.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.AudioCodec.Width = 120;
            // 
            // Source
            // 
            this.Source.DataPropertyName = "SourceTrack";
            this.Source.FillWeight = 49.69727F;
            this.Source.HeaderText = "Source";
            this.Source.Name = "Source";
            this.Source.ReadOnly = true;
            this.Source.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Source.Width = 170;
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
            this.DRC});
            this.audioList.ContextMenuStrip = this.audioMenu;
            this.audioList.GridColor = System.Drawing.Color.White;
            this.audioList.Location = new System.Drawing.Point(16, 114);
            this.audioList.Name = "audioList";
            this.audioList.RowHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.audioList.RowHeadersVisible = false;
            this.audioList.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.AutoSizeToAllHeaders;
            this.audioList.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.audioList.ShowCellErrors = false;
            this.audioList.ShowCellToolTips = false;
            this.audioList.ShowEditingIcon = false;
            this.audioList.ShowRowErrors = false;
            this.audioList.Size = new System.Drawing.Size(685, 180);
            this.audioList.TabIndex = 67;
            this.ToolTips.SetToolTip(this.audioList, "The audio tracks to be encoded into the output file.");
            this.audioList.SelectionChanged += new System.EventHandler(this.audioList_SelectionChanged);
            // 
            // AudioPanel
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lbl_audioTrack);
            this.Controls.Add(this.drp_audioTrack);
            this.Controls.Add(this.drp_audioSample);
            this.Controls.Add(this.audioList);
            this.Controls.Add(this.label68);
            this.Controls.Add(this.drp_audioBitrate);
            this.Controls.Add(this.drp_audioMix);
            this.Controls.Add(this.btn_addAudioTrack);
            this.Controls.Add(this.lbl_drc);
            this.Controls.Add(this.btn_RemoveAudioTrack);
            this.Controls.Add(this.drp_audioEncoder);
            this.Controls.Add(this.tb_drc);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "AudioPanel";
            this.Size = new System.Drawing.Size(720, 310);
            this.audioMenu.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.tb_drc)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.audioList)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_RemoveAudioTrack;
        private System.Windows.Forms.Button btn_addAudioTrack;
        internal System.Windows.Forms.Label lbl_drc;
        internal System.Windows.Forms.TrackBar tb_drc;
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
        internal System.Windows.Forms.Label label1;
        internal System.Windows.Forms.Label lbl_audioTrack;
        private System.Windows.Forms.DataGridViewTextBoxColumn DRC;
        private System.Windows.Forms.DataGridViewTextBoxColumn Bitrate;
        private System.Windows.Forms.DataGridViewTextBoxColumn Samplerate;
        private System.Windows.Forms.DataGridViewTextBoxColumn Mixdown;
        private System.Windows.Forms.DataGridViewTextBoxColumn AudioCodec;
        private System.Windows.Forms.DataGridViewTextBoxColumn Source;
        private System.Windows.Forms.DataGridView audioList;
        private System.Windows.Forms.ToolTip ToolTips;
    }
}
