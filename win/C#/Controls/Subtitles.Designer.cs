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
            this.components = new System.ComponentModel.Container();
            this.drp_subtitleTracks = new System.Windows.Forms.ComboBox();
            this.SubTitlesGroup = new System.Windows.Forms.GroupBox();
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
            this.type = new System.Windows.Forms.ColumnHeader();
            this.srtOffset = new System.Windows.Forms.ColumnHeader();
            this.subMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_moveup = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_movedown = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_remove = new System.Windows.Forms.ToolStripMenuItem();
            this.label68 = new System.Windows.Forms.Label();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.SRTGroup = new System.Windows.Forms.GroupBox();
            this.label6 = new System.Windows.Forms.Label();
            this.check_default_srt = new System.Windows.Forms.CheckBox();
            this.srt_offset = new System.Windows.Forms.NumericUpDown();
            this.label5 = new System.Windows.Forms.Label();
            this.srt_browse = new System.Windows.Forms.Button();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.srt_charcode = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.srt_lang = new System.Windows.Forms.ComboBox();
            this.btn_srtAdd = new System.Windows.Forms.Button();
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.SubTitlesGroup.SuspendLayout();
            this.subMenu.SuspendLayout();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.SRTGroup.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.srt_offset)).BeginInit();
            this.SuspendLayout();
            // 
            // drp_subtitleTracks
            // 
            this.drp_subtitleTracks.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_subtitleTracks.FormattingEnabled = true;
            this.drp_subtitleTracks.Location = new System.Drawing.Point(57, 20);
            this.drp_subtitleTracks.Name = "drp_subtitleTracks";
            this.drp_subtitleTracks.Size = new System.Drawing.Size(194, 21);
            this.drp_subtitleTracks.TabIndex = 50;
            this.drp_subtitleTracks.SelectedIndexChanged += new System.EventHandler(this.drp_subtitleTracks_SelectedIndexChanged);
            // 
            // SubTitlesGroup
            // 
            this.SubTitlesGroup.BackColor = System.Drawing.Color.Transparent;
            this.SubTitlesGroup.Controls.Add(this.label1);
            this.SubTitlesGroup.Controls.Add(this.check_default);
            this.SubTitlesGroup.Controls.Add(this.check_burned);
            this.SubTitlesGroup.Controls.Add(this.check_forced);
            this.SubTitlesGroup.Controls.Add(this.drp_subtitleTracks);
            this.SubTitlesGroup.ForeColor = System.Drawing.Color.Black;
            this.SubTitlesGroup.Location = new System.Drawing.Point(6, 6);
            this.SubTitlesGroup.Name = "SubTitlesGroup";
            this.SubTitlesGroup.Size = new System.Drawing.Size(669, 50);
            this.SubTitlesGroup.TabIndex = 71;
            this.SubTitlesGroup.TabStop = false;
            this.SubTitlesGroup.Text = "Selected Track: New Track";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 23);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(37, 13);
            this.label1.TabIndex = 72;
            this.label1.Text = "Track:";
            // 
            // check_default
            // 
            this.check_default.AutoSize = true;
            this.check_default.Location = new System.Drawing.Point(426, 22);
            this.check_default.Name = "check_default";
            this.check_default.Size = new System.Drawing.Size(61, 17);
            this.check_default.TabIndex = 71;
            this.check_default.Text = "Default";
            this.check_default.UseVisualStyleBackColor = true;
            this.check_default.CheckedChanged += new System.EventHandler(this.check_default_CheckedChanged);
            // 
            // check_burned
            // 
            this.check_burned.AutoSize = true;
            this.check_burned.Location = new System.Drawing.Point(347, 22);
            this.check_burned.Name = "check_burned";
            this.check_burned.Size = new System.Drawing.Size(73, 17);
            this.check_burned.TabIndex = 70;
            this.check_burned.Text = "Burned In";
            this.check_burned.UseVisualStyleBackColor = true;
            this.check_burned.CheckedChanged += new System.EventHandler(this.check_burned_CheckedChanged);
            // 
            // check_forced
            // 
            this.check_forced.AutoSize = true;
            this.check_forced.Location = new System.Drawing.Point(257, 22);
            this.check_forced.Name = "check_forced";
            this.check_forced.Size = new System.Drawing.Size(84, 17);
            this.check_forced.TabIndex = 69;
            this.check_forced.Text = "Forced Only";
            this.check_forced.UseVisualStyleBackColor = true;
            this.check_forced.CheckedChanged += new System.EventHandler(this.check_forced_CheckedChanged);
            // 
            // btn_addSubTrack
            // 
            this.btn_addSubTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_addSubTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_addSubTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_addSubTrack.Location = new System.Drawing.Point(16, 38);
            this.btn_addSubTrack.Name = "btn_addSubTrack";
            this.btn_addSubTrack.Size = new System.Drawing.Size(78, 23);
            this.btn_addSubTrack.TabIndex = 68;
            this.btn_addSubTrack.Text = "Add Track";
            this.btn_addSubTrack.UseVisualStyleBackColor = false;
            this.btn_addSubTrack.Click += new System.EventHandler(this.btn_addSubTrack_Click);
            // 
            // btn_RemoveSubTrack
            // 
            this.btn_RemoveSubTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_RemoveSubTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_RemoveSubTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_RemoveSubTrack.Location = new System.Drawing.Point(230, 38);
            this.btn_RemoveSubTrack.Name = "btn_RemoveSubTrack";
            this.btn_RemoveSubTrack.Size = new System.Drawing.Size(73, 23);
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
            this.defaultTrack,
            this.type,
            this.srtOffset});
            this.lv_subList.ContextMenuStrip = this.subMenu;
            this.lv_subList.FullRowSelect = true;
            this.lv_subList.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.lv_subList.HideSelection = false;
            this.lv_subList.LabelWrap = false;
            this.lv_subList.Location = new System.Drawing.Point(16, 169);
            this.lv_subList.MultiSelect = false;
            this.lv_subList.Name = "lv_subList";
            this.lv_subList.Size = new System.Drawing.Size(688, 114);
            this.lv_subList.TabIndex = 70;
            this.lv_subList.UseCompatibleStateImageBehavior = false;
            this.lv_subList.View = System.Windows.Forms.View.Details;
            this.lv_subList.SelectedIndexChanged += new System.EventHandler(this.lb_subList_SelectedIndexChanged);
            // 
            // id
            // 
            this.id.Text = "Track ID";
            this.id.Width = 65;
            // 
            // col_Source
            // 
            this.col_Source.Text = "Track";
            this.col_Source.Width = 150;
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
            // type
            // 
            this.type.Text = "Type";
            this.type.Width = 110;
            // 
            // srtOffset
            // 
            this.srtOffset.Text = "Srt Offset";
            this.srtOffset.Width = 70;
            // 
            // subMenu
            // 
            this.subMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_moveup,
            this.mnu_movedown,
            this.toolStripSeparator2,
            this.mnu_remove});
            this.subMenu.Name = "audioMenu";
            this.subMenu.Size = new System.Drawing.Size(142, 76);
            // 
            // mnu_moveup
            // 
            this.mnu_moveup.Name = "mnu_moveup";
            this.mnu_moveup.Size = new System.Drawing.Size(141, 22);
            this.mnu_moveup.Text = "Move Up";
            this.mnu_moveup.Click += new System.EventHandler(this.mnu_moveup_Click);
            // 
            // mnu_movedown
            // 
            this.mnu_movedown.Name = "mnu_movedown";
            this.mnu_movedown.Size = new System.Drawing.Size(141, 22);
            this.mnu_movedown.Text = "Move Down";
            this.mnu_movedown.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.mnu_movedown.Click += new System.EventHandler(this.mnu_movedown_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(138, 6);
            // 
            // mnu_remove
            // 
            this.mnu_remove.Name = "mnu_remove";
            this.mnu_remove.Size = new System.Drawing.Size(141, 22);
            this.mnu_remove.Text = "Remove";
            this.mnu_remove.Click += new System.EventHandler(this.mnu_remove_Click);
            // 
            // label68
            // 
            this.label68.AutoSize = true;
            this.label68.BackColor = System.Drawing.Color.Transparent;
            this.label68.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label68.Location = new System.Drawing.Point(13, 13);
            this.label68.Name = "label68";
            this.label68.Size = new System.Drawing.Size(57, 13);
            this.label68.TabIndex = 67;
            this.label68.Text = "Subtitles";
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Location = new System.Drawing.Point(15, 71);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(689, 92);
            this.tabControl1.SizeMode = System.Windows.Forms.TabSizeMode.Fixed;
            this.tabControl1.TabIndex = 72;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.SubTitlesGroup);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(681, 66);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Subtitles";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.SRTGroup);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(681, 66);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "External SRT";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // SRTGroup
            // 
            this.SRTGroup.BackColor = System.Drawing.Color.Transparent;
            this.SRTGroup.Controls.Add(this.label6);
            this.SRTGroup.Controls.Add(this.check_default_srt);
            this.SRTGroup.Controls.Add(this.srt_offset);
            this.SRTGroup.Controls.Add(this.label5);
            this.SRTGroup.Controls.Add(this.srt_browse);
            this.SRTGroup.Controls.Add(this.label4);
            this.SRTGroup.Controls.Add(this.label3);
            this.SRTGroup.Controls.Add(this.srt_charcode);
            this.SRTGroup.Controls.Add(this.label2);
            this.SRTGroup.Controls.Add(this.srt_lang);
            this.SRTGroup.ForeColor = System.Drawing.Color.Black;
            this.SRTGroup.Location = new System.Drawing.Point(6, 6);
            this.SRTGroup.Name = "SRTGroup";
            this.SRTGroup.Size = new System.Drawing.Size(669, 50);
            this.SRTGroup.TabIndex = 72;
            this.SRTGroup.TabStop = false;
            this.SRTGroup.Text = "Selected Track: New Track";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(466, 23);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(20, 13);
            this.label6.TabIndex = 81;
            this.label6.Text = "ms";
            // 
            // check_default_srt
            // 
            this.check_default_srt.AutoSize = true;
            this.check_default_srt.Location = new System.Drawing.Point(491, 22);
            this.check_default_srt.Name = "check_default_srt";
            this.check_default_srt.Size = new System.Drawing.Size(61, 17);
            this.check_default_srt.TabIndex = 80;
            this.check_default_srt.Text = "Default";
            this.check_default_srt.UseVisualStyleBackColor = true;
            this.check_default_srt.CheckedChanged += new System.EventHandler(this.check_default_srt_CheckedChanged);
            // 
            // srt_offset
            // 
            this.srt_offset.Increment = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.srt_offset.Location = new System.Drawing.Point(402, 20);
            this.srt_offset.Maximum = new decimal(new int[] {
            1000000000,
            0,
            0,
            0});
            this.srt_offset.Name = "srt_offset";
            this.srt_offset.Size = new System.Drawing.Size(58, 21);
            this.srt_offset.TabIndex = 79;
            this.srt_offset.ValueChanged += new System.EventHandler(this.srt_offset_ValueChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(555, 23);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(27, 13);
            this.label5.TabIndex = 78;
            this.label5.Text = "File:";
            // 
            // srt_browse
            // 
            this.srt_browse.BackColor = System.Drawing.Color.Transparent;
            this.srt_browse.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.srt_browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.srt_browse.Location = new System.Drawing.Point(588, 18);
            this.srt_browse.Name = "srt_browse";
            this.srt_browse.Size = new System.Drawing.Size(67, 23);
            this.srt_browse.TabIndex = 77;
            this.srt_browse.Text = "Browse";
            this.srt_browse.UseVisualStyleBackColor = false;
            this.srt_browse.Click += new System.EventHandler(this.srt_browse_Click);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(354, 23);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(42, 13);
            this.label4.TabIndex = 76;
            this.label4.Text = "Offset:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(202, 23);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(62, 13);
            this.label3.TabIndex = 74;
            this.label3.Text = "Char Code:";
            // 
            // srt_charcode
            // 
            this.srt_charcode.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.srt_charcode.FormattingEnabled = true;
            this.srt_charcode.Items.AddRange(new object[] {
            "ANSI_X3.4-1968",
            "ANSI_X3.4-1986",
            "ANSI_X3.4",
            "ANSI_X3.110-1983",
            "ANSI_X3.110",
            "ASCII",
            "ECMA-114",
            "ECMA-118",
            "ECMA-128",
            "ECMA-CYRILLIC",
            "IEC_P27-1",
            "ISO-8859-1",
            "ISO-8859-2",
            "ISO-8859-3",
            "ISO-8859-4",
            "ISO-8859-5",
            "ISO-8859-6",
            "ISO-8859-7",
            "ISO-8859-8",
            "ISO-8859-9",
            "ISO-8859-9E",
            "ISO-8859-10",
            "ISO-8859-11",
            "ISO-8859-13",
            "ISO-8859-14",
            "ISO-8859-15",
            "ISO-8859-16",
            "UTF-7",
            "UTF-8",
            "UTF-16",
            "UTF-32"});
            this.srt_charcode.Location = new System.Drawing.Point(270, 20);
            this.srt_charcode.Name = "srt_charcode";
            this.srt_charcode.Size = new System.Drawing.Size(78, 21);
            this.srt_charcode.TabIndex = 73;
            this.srt_charcode.SelectedIndexChanged += new System.EventHandler(this.srt_charcode_SelectedIndexChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 23);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(58, 13);
            this.label2.TabIndex = 72;
            this.label2.Text = "Language:";
            // 
            // srt_lang
            // 
            this.srt_lang.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.srt_lang.FormattingEnabled = true;
            this.srt_lang.Location = new System.Drawing.Point(70, 20);
            this.srt_lang.Name = "srt_lang";
            this.srt_lang.Size = new System.Drawing.Size(126, 21);
            this.srt_lang.TabIndex = 50;
            this.srt_lang.SelectedIndexChanged += new System.EventHandler(this.srt_lang_SelectedIndexChanged);
            // 
            // btn_srtAdd
            // 
            this.btn_srtAdd.BackColor = System.Drawing.Color.Transparent;
            this.btn_srtAdd.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_srtAdd.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_srtAdd.Location = new System.Drawing.Point(100, 38);
            this.btn_srtAdd.Name = "btn_srtAdd";
            this.btn_srtAdd.Size = new System.Drawing.Size(124, 23);
            this.btn_srtAdd.TabIndex = 73;
            this.btn_srtAdd.Text = "Add External SRT";
            this.btn_srtAdd.UseVisualStyleBackColor = false;
            this.btn_srtAdd.Click += new System.EventHandler(this.btn_srtAdd_Click);
            // 
            // openFileDialog
            // 
            this.openFileDialog.DefaultExt = "srt";
            this.openFileDialog.Filter = "SRT Files |*.srt";
            // 
            // Subtitles
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.btn_srtAdd);
            this.Controls.Add(this.btn_RemoveSubTrack);
            this.Controls.Add(this.label68);
            this.Controls.Add(this.tabControl1);
            this.Controls.Add(this.lv_subList);
            this.Controls.Add(this.btn_addSubTrack);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "Subtitles";
            this.Size = new System.Drawing.Size(719, 300);
            this.SubTitlesGroup.ResumeLayout(false);
            this.SubTitlesGroup.PerformLayout();
            this.subMenu.ResumeLayout(false);
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.SRTGroup.ResumeLayout(false);
            this.SRTGroup.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.srt_offset)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.ComboBox drp_subtitleTracks;
        private System.Windows.Forms.GroupBox SubTitlesGroup;
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
        private System.Windows.Forms.ContextMenuStrip subMenu;
        private System.Windows.Forms.ToolStripMenuItem mnu_moveup;
        private System.Windows.Forms.ToolStripMenuItem mnu_movedown;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem mnu_remove;
        internal System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.Button btn_srtAdd;
        private System.Windows.Forms.ColumnHeader type;
        private System.Windows.Forms.ColumnHeader srtOffset;
        private System.Windows.Forms.GroupBox SRTGroup;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Button srt_browse;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        internal System.Windows.Forms.ComboBox srt_charcode;
        private System.Windows.Forms.Label label2;
        internal System.Windows.Forms.ComboBox srt_lang;
        private System.Windows.Forms.NumericUpDown srt_offset;
        private System.Windows.Forms.OpenFileDialog openFileDialog;
        private System.Windows.Forms.CheckBox check_default_srt;
        private System.Windows.Forms.Label label6;
    }
}
