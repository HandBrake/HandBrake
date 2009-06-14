namespace Handbrake.Controls
{
    partial class Subtitles
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
            this.drp_subtitleTracks = new System.Windows.Forms.ComboBox();
            this.AudioTrackGroup = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.check_default = new System.Windows.Forms.CheckBox();
            this.check_burned = new System.Windows.Forms.CheckBox();
            this.check_forced = new System.Windows.Forms.CheckBox();
            this.btn_addSubTrack = new System.Windows.Forms.Button();
            this.btn_RemoveSubTrack = new System.Windows.Forms.Button();
            this.lv_subList = new System.Windows.Forms.ListView();
            this.id = new System.Windows.Forms.ColumnHeader();
            this.col_Source = new System.Windows.Forms.ColumnHeader();
            this.forced = new System.Windows.Forms.ColumnHeader();
            this.burned = new System.Windows.Forms.ColumnHeader();
            this.defaultTrack = new System.Windows.Forms.ColumnHeader();
            this.label68 = new System.Windows.Forms.Label();
            this.AudioTrackGroup.SuspendLayout();
            this.SuspendLayout();
            // 
            // drp_subtitleTracks
            // 
            this.drp_subtitleTracks.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_subtitleTracks.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_subtitleTracks.FormattingEnabled = true;
            this.drp_subtitleTracks.Location = new System.Drawing.Point(57, 20);
            this.drp_subtitleTracks.Name = "drp_subtitleTracks";
            this.drp_subtitleTracks.Size = new System.Drawing.Size(194, 20);
            this.drp_subtitleTracks.TabIndex = 50;
            this.drp_subtitleTracks.SelectedIndexChanged += new System.EventHandler(this.drp_subtitleTracks_SelectedIndexChanged);
            // 
            // AudioTrackGroup
            // 
            this.AudioTrackGroup.BackColor = System.Drawing.Color.Transparent;
            this.AudioTrackGroup.Controls.Add(this.label1);
            this.AudioTrackGroup.Controls.Add(this.check_default);
            this.AudioTrackGroup.Controls.Add(this.check_burned);
            this.AudioTrackGroup.Controls.Add(this.check_forced);
            this.AudioTrackGroup.Controls.Add(this.drp_subtitleTracks);
            this.AudioTrackGroup.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.AudioTrackGroup.ForeColor = System.Drawing.Color.Black;
            this.AudioTrackGroup.Location = new System.Drawing.Point(16, 32);
            this.AudioTrackGroup.Name = "AudioTrackGroup";
            this.AudioTrackGroup.Size = new System.Drawing.Size(689, 50);
            this.AudioTrackGroup.TabIndex = 71;
            this.AudioTrackGroup.TabStop = false;
            this.AudioTrackGroup.Text = "Selected Track: New Track";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(6, 22);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(44, 13);
            this.label1.TabIndex = 72;
            this.label1.Text = "Track:";
            // 
            // check_default
            // 
            this.check_default.AutoSize = true;
            this.check_default.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_default.Location = new System.Drawing.Point(446, 21);
            this.check_default.Name = "check_default";
            this.check_default.Size = new System.Drawing.Size(67, 17);
            this.check_default.TabIndex = 71;
            this.check_default.Text = "Default";
            this.check_default.UseVisualStyleBackColor = true;
            this.check_default.CheckedChanged += new System.EventHandler(this.check_default_CheckedChanged);
            // 
            // check_burned
            // 
            this.check_burned.AutoSize = true;
            this.check_burned.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_burned.Location = new System.Drawing.Point(357, 21);
            this.check_burned.Name = "check_burned";
            this.check_burned.Size = new System.Drawing.Size(83, 17);
            this.check_burned.TabIndex = 70;
            this.check_burned.Text = "Burned In";
            this.check_burned.UseVisualStyleBackColor = true;
            this.check_burned.CheckedChanged += new System.EventHandler(this.check_burned_CheckedChanged);
            // 
            // check_forced
            // 
            this.check_forced.AutoSize = true;
            this.check_forced.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_forced.Location = new System.Drawing.Point(257, 21);
            this.check_forced.Name = "check_forced";
            this.check_forced.Size = new System.Drawing.Size(94, 17);
            this.check_forced.TabIndex = 69;
            this.check_forced.Text = "Forced Only";
            this.check_forced.UseVisualStyleBackColor = true;
            this.check_forced.CheckedChanged += new System.EventHandler(this.check_forced_CheckedChanged);
            // 
            // btn_addSubTrack
            // 
            this.btn_addSubTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_addSubTrack.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_addSubTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_addSubTrack.Location = new System.Drawing.Point(16, 274);
            this.btn_addSubTrack.Name = "btn_addSubTrack";
            this.btn_addSubTrack.Size = new System.Drawing.Size(111, 23);
            this.btn_addSubTrack.TabIndex = 68;
            this.btn_addSubTrack.Text = "Add Track";
            this.btn_addSubTrack.UseVisualStyleBackColor = false;
            this.btn_addSubTrack.Click += new System.EventHandler(this.btn_addSubTrack_Click);
            // 
            // btn_RemoveSubTrack
            // 
            this.btn_RemoveSubTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_RemoveSubTrack.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_RemoveSubTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_RemoveSubTrack.Location = new System.Drawing.Point(133, 274);
            this.btn_RemoveSubTrack.Name = "btn_RemoveSubTrack";
            this.btn_RemoveSubTrack.Size = new System.Drawing.Size(111, 23);
            this.btn_RemoveSubTrack.TabIndex = 69;
            this.btn_RemoveSubTrack.Text = "Remove";
            this.btn_RemoveSubTrack.UseVisualStyleBackColor = false;
            this.btn_RemoveSubTrack.Click += new System.EventHandler(this.btn_RemoveSubTrack_Click);
            // 
            // lv_subList
            // 
            this.lv_subList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.id,
            this.col_Source,
            this.forced,
            this.burned,
            this.defaultTrack});
            this.lv_subList.FullRowSelect = true;
            this.lv_subList.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.lv_subList.HideSelection = false;
            this.lv_subList.LabelWrap = false;
            this.lv_subList.Location = new System.Drawing.Point(17, 88);
            this.lv_subList.MultiSelect = false;
            this.lv_subList.Name = "lv_subList";
            this.lv_subList.Size = new System.Drawing.Size(688, 180);
            this.lv_subList.TabIndex = 70;
            this.lv_subList.UseCompatibleStateImageBehavior = false;
            this.lv_subList.View = System.Windows.Forms.View.Details;
            this.lv_subList.SelectedIndexChanged += new System.EventHandler(this.lb_subList_SelectedIndexChanged);
            // 
            // id
            // 
            this.id.Text = "Track ID";
            this.id.Width = 70;
            // 
            // col_Source
            // 
            this.col_Source.Text = "Track";
            this.col_Source.Width = 250;
            // 
            // forced
            // 
            this.forced.Text = "Forced Only";
            this.forced.Width = 88;
            // 
            // burned
            // 
            this.burned.Text = "Burned In";
            this.burned.Width = 100;
            // 
            // defaultTrack
            // 
            this.defaultTrack.Text = "Default";
            this.defaultTrack.Width = 100;
            // 
            // label68
            // 
            this.label68.AutoSize = true;
            this.label68.BackColor = System.Drawing.Color.Transparent;
            this.label68.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label68.Location = new System.Drawing.Point(13, 13);
            this.label68.Name = "label68";
            this.label68.Size = new System.Drawing.Size(64, 13);
            this.label68.TabIndex = 67;
            this.label68.Text = "Subtitles";
            // 
            // Subtitles
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.AudioTrackGroup);
            this.Controls.Add(this.btn_RemoveSubTrack);
            this.Controls.Add(this.lv_subList);
            this.Controls.Add(this.label68);
            this.Controls.Add(this.btn_addSubTrack);
            this.Name = "Subtitles";
            this.Size = new System.Drawing.Size(719, 300);
            this.AudioTrackGroup.ResumeLayout(false);
            this.AudioTrackGroup.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.ComboBox drp_subtitleTracks;
        private System.Windows.Forms.GroupBox AudioTrackGroup;
        private System.Windows.Forms.Button btn_RemoveSubTrack;
        private System.Windows.Forms.Button btn_addSubTrack;
        internal System.Windows.Forms.ListView lv_subList;
        private System.Windows.Forms.ColumnHeader id;
        private System.Windows.Forms.ColumnHeader col_Source;
        private System.Windows.Forms.ColumnHeader forced;
        private System.Windows.Forms.ColumnHeader burned;
        internal System.Windows.Forms.Label label68;
        private System.Windows.Forms.ColumnHeader defaultTrack;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox check_default;
        private System.Windows.Forms.CheckBox check_burned;
        private System.Windows.Forms.CheckBox check_forced;
    }
}
