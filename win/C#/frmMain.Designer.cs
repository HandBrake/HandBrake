/*  frmMain.Designer.cs 
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmMain
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.Windows.Forms.Label Label38;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmMain));
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            this.DVD_Save = new System.Windows.Forms.SaveFileDialog();
            this.File_Save = new System.Windows.Forms.SaveFileDialog();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.drop_chapterFinish = new System.Windows.Forms.ComboBox();
            this.drop_chapterStart = new System.Windows.Forms.ComboBox();
            this.drp_dvdtitle = new System.Windows.Forms.ComboBox();
            this.text_source = new System.Windows.Forms.TextBox();
            this.text_destination = new System.Windows.Forms.TextBox();
            this.drp_videoEncoder = new System.Windows.Forms.ComboBox();
            this.drp_audbit_1 = new System.Windows.Forms.ComboBox();
            this.drp_audsr_1 = new System.Windows.Forms.ComboBox();
            this.check_largeFile = new System.Windows.Forms.CheckBox();
            this.check_turbo = new System.Windows.Forms.CheckBox();
            this.drp_videoFramerate = new System.Windows.Forms.ComboBox();
            this.slider_videoQuality = new System.Windows.Forms.TrackBar();
            this.text_filesize = new System.Windows.Forms.TextBox();
            this.text_bitrate = new System.Windows.Forms.TextBox();
            this.drp_subtitle = new System.Windows.Forms.ComboBox();
            this.btn_setDefault = new System.Windows.Forms.Button();
            this.drp_audmix_1 = new System.Windows.Forms.ComboBox();
            this.text_height = new System.Windows.Forms.TextBox();
            this.text_width = new System.Windows.Forms.TextBox();
            this.check_optimiseMP4 = new System.Windows.Forms.CheckBox();
            this.check_iPodAtom = new System.Windows.Forms.CheckBox();
            this.data_chpt = new System.Windows.Forms.DataGridView();
            this.number = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.name = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.btn_addPreset = new System.Windows.Forms.Button();
            this.btn_removePreset = new System.Windows.Forms.Button();
            this.drp_audmix_2 = new System.Windows.Forms.ComboBox();
            this.drp_audenc_1 = new System.Windows.Forms.ComboBox();
            this.drp_audenc_2 = new System.Windows.Forms.ComboBox();
            this.drp_audbit_2 = new System.Windows.Forms.ComboBox();
            this.drp_audsr_2 = new System.Windows.Forms.ComboBox();
            this.check_fileMode = new System.Windows.Forms.CheckBox();
            this.slider_drc = new System.Windows.Forms.TrackBar();
            this.DVD_Open = new System.Windows.Forms.FolderBrowserDialog();
            this.File_Open = new System.Windows.Forms.OpenFileDialog();
            this.ISO_Open = new System.Windows.Forms.OpenFileDialog();
            this.FileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_open = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_save = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_exit = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_open3 = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_encode = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_viewDVDdata = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_options = new System.Windows.Forms.ToolStripMenuItem();
            this.PresetsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_SelectDefault = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator7 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_presetReset = new System.Windows.Forms.ToolStripMenuItem();
            this.HelpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.OnlineDocumentationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_wiki = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_faq = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_onlineDocs = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_handbrake_home = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_UpdateCheck = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_about = new System.Windows.Forms.ToolStripMenuItem();
            this.frmMainMenu = new System.Windows.Forms.MenuStrip();
            this.GroupBox1 = new System.Windows.Forms.GroupBox();
            this.btn_Browse = new System.Windows.Forms.Button();
            this.Label13 = new System.Windows.Forms.Label();
            this.Label17 = new System.Windows.Forms.Label();
            this.Label9 = new System.Windows.Forms.Label();
            this.Label10 = new System.Windows.Forms.Label();
            this.groupBox_output = new System.Windows.Forms.GroupBox();
            this.Label47 = new System.Windows.Forms.Label();
            this.Label3 = new System.Windows.Forms.Label();
            this.lbl_encode = new System.Windows.Forms.Label();
            this.TabPage2 = new System.Windows.Forms.TabPage();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.label12 = new System.Windows.Forms.Label();
            this.label68 = new System.Windows.Forms.Label();
            this.label67 = new System.Windows.Forms.Label();
            this.label66 = new System.Windows.Forms.Label();
            this.label65 = new System.Windows.Forms.Label();
            this.label14 = new System.Windows.Forms.Label();
            this.label30 = new System.Windows.Forms.Label();
            this.label29 = new System.Windows.Forms.Label();
            this.label23 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.lbl_drc = new System.Windows.Forms.Label();
            this.check_forced = new System.Windows.Forms.CheckBox();
            this.drp_track2Audio = new System.Windows.Forms.ComboBox();
            this.label28 = new System.Windows.Forms.Label();
            this.label27 = new System.Windows.Forms.Label();
            this.Label19 = new System.Windows.Forms.Label();
            this.Label20 = new System.Windows.Forms.Label();
            this.drp_track1Audio = new System.Windows.Forms.ComboBox();
            this.Label5 = new System.Windows.Forms.Label();
            this.Label32 = new System.Windows.Forms.Label();
            this.TabPage3 = new System.Windows.Forms.TabPage();
            this.label25 = new System.Windows.Forms.Label();
            this.lbl_vfr = new System.Windows.Forms.Label();
            this.check_grayscale = new System.Windows.Forms.CheckBox();
            this.Label22 = new System.Windows.Forms.Label();
            this.check_2PassEncode = new System.Windows.Forms.CheckBox();
            this.Label2 = new System.Windows.Forms.Label();
            this.Label42 = new System.Windows.Forms.Label();
            this.SliderValue = new System.Windows.Forms.Label();
            this.Label46 = new System.Windows.Forms.Label();
            this.Label40 = new System.Windows.Forms.Label();
            this.TabPage1 = new System.Windows.Forms.TabPage();
            this.label6 = new System.Windows.Forms.Label();
            this.drp_anamorphic = new System.Windows.Forms.ComboBox();
            this.text_bottom = new System.Windows.Forms.NumericUpDown();
            this.text_top = new System.Windows.Forms.NumericUpDown();
            this.text_left = new System.Windows.Forms.NumericUpDown();
            this.text_right = new System.Windows.Forms.NumericUpDown();
            this.label26 = new System.Windows.Forms.Label();
            this.Label56 = new System.Windows.Forms.Label();
            this.lbl_Aspect = new System.Windows.Forms.Label();
            this.Label91 = new System.Windows.Forms.Label();
            this.Label55 = new System.Windows.Forms.Label();
            this.check_vfr = new System.Windows.Forms.CheckBox();
            this.label24 = new System.Windows.Forms.Label();
            this.drp_deNoise = new System.Windows.Forms.ComboBox();
            this.label11 = new System.Windows.Forms.Label();
            this.check_deblock = new System.Windows.Forms.CheckBox();
            this.check_detelecine = new System.Windows.Forms.CheckBox();
            this.label4 = new System.Windows.Forms.Label();
            this.drp_deInterlace_option = new System.Windows.Forms.ComboBox();
            this.lbl_RecomendedCrop = new System.Windows.Forms.Label();
            this.Label8 = new System.Windows.Forms.Label();
            this.Label1 = new System.Windows.Forms.Label();
            this.Label53 = new System.Windows.Forms.Label();
            this.Label52 = new System.Windows.Forms.Label();
            this.Label51 = new System.Windows.Forms.Label();
            this.Label50 = new System.Windows.Forms.Label();
            this.Label15 = new System.Windows.Forms.Label();
            this.drp_crop = new System.Windows.Forms.ComboBox();
            this.Check_ChapterMarkers = new System.Windows.Forms.CheckBox();
            this.advancedOptions = new System.Windows.Forms.TabControl();
            this.tab_chapters = new System.Windows.Forms.TabPage();
            this.label31 = new System.Windows.Forms.Label();
            this.h264Tab = new System.Windows.Forms.TabPage();
            this.label43 = new System.Windows.Forms.Label();
            this.btn_reset = new System.Windows.Forms.Button();
            this.rtf_x264Query = new System.Windows.Forms.RichTextBox();
            this.label35 = new System.Windows.Forms.Label();
            this.check_Cabac = new System.Windows.Forms.CheckBox();
            this.label36 = new System.Windows.Forms.Label();
            this.check_noDCTDecimate = new System.Windows.Forms.CheckBox();
            this.label37 = new System.Windows.Forms.Label();
            this.check_noFastPSkip = new System.Windows.Forms.CheckBox();
            this.label39 = new System.Windows.Forms.Label();
            this.drop_trellis = new System.Windows.Forms.ComboBox();
            this.drop_deblockBeta = new System.Windows.Forms.ComboBox();
            this.label41 = new System.Windows.Forms.Label();
            this.drop_deblockAlpha = new System.Windows.Forms.ComboBox();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.label44 = new System.Windows.Forms.Label();
            this.check_8x8DCT = new System.Windows.Forms.CheckBox();
            this.label45 = new System.Windows.Forms.Label();
            this.drop_analysis = new System.Windows.Forms.ComboBox();
            this.label48 = new System.Windows.Forms.Label();
            this.drop_subpixelMotionEstimation = new System.Windows.Forms.ComboBox();
            this.label49 = new System.Windows.Forms.Label();
            this.drop_MotionEstimationRange = new System.Windows.Forms.ComboBox();
            this.label54 = new System.Windows.Forms.Label();
            this.drop_MotionEstimationMethod = new System.Windows.Forms.ComboBox();
            this.label57 = new System.Windows.Forms.Label();
            this.check_pyrmidalBFrames = new System.Windows.Forms.CheckBox();
            this.label58 = new System.Windows.Forms.Label();
            this.check_BidirectionalRefinement = new System.Windows.Forms.CheckBox();
            this.label59 = new System.Windows.Forms.Label();
            this.check_bFrameDistortion = new System.Windows.Forms.CheckBox();
            this.label60 = new System.Windows.Forms.Label();
            this.check_weightedBFrames = new System.Windows.Forms.CheckBox();
            this.label61 = new System.Windows.Forms.Label();
            this.drop_directPrediction = new System.Windows.Forms.ComboBox();
            this.label62 = new System.Windows.Forms.Label();
            this.drop_bFrames = new System.Windows.Forms.ComboBox();
            this.label63 = new System.Windows.Forms.Label();
            this.label64 = new System.Windows.Forms.Label();
            this.drop_refFrames = new System.Windows.Forms.ComboBox();
            this.check_mixedReferences = new System.Windows.Forms.CheckBox();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.btn_clear = new System.Windows.Forms.Button();
            this.btn_copy2C = new System.Windows.Forms.Button();
            this.label34 = new System.Windows.Forms.Label();
            this.btn_generate_Query = new System.Windows.Forms.Button();
            this.label33 = new System.Windows.Forms.Label();
            this.rtf_query = new System.Windows.Forms.RichTextBox();
            this.groupBox_dest = new System.Windows.Forms.GroupBox();
            this.btn_destBrowse = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.treeView_presets = new System.Windows.Forms.TreeView();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.btn_start = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_add2Queue = new System.Windows.Forms.ToolStripButton();
            this.btn_showQueue = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_ActivityWindow = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator8 = new System.Windows.Forms.ToolStripSeparator();
            this.button1 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            Label38 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.data_chpt)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_drc)).BeginInit();
            this.frmMainMenu.SuspendLayout();
            this.GroupBox1.SuspendLayout();
            this.groupBox_output.SuspendLayout();
            this.TabPage2.SuspendLayout();
            this.TabPage3.SuspendLayout();
            this.TabPage1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.text_bottom)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_top)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_left)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_right)).BeginInit();
            this.advancedOptions.SuspendLayout();
            this.tab_chapters.SuspendLayout();
            this.h264Tab.SuspendLayout();
            this.tabPage4.SuspendLayout();
            this.groupBox_dest.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // Label38
            // 
            Label38.AutoSize = true;
            Label38.BackColor = System.Drawing.Color.Transparent;
            Label38.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            Label38.Location = new System.Drawing.Point(304, 65);
            Label38.Name = "Label38";
            Label38.Size = new System.Drawing.Size(108, 13);
            Label38.TabIndex = 11;
            Label38.Text = "Target Size (MB):";
            // 
            // DVD_Save
            // 
            this.DVD_Save.Filter = "mp4|*.mp4|m4v|*.m4v|avi|*.avi|ogm|*.ogm|mkv|*.mkv";
            // 
            // File_Save
            // 
            this.File_Save.DefaultExt = "hb";
            this.File_Save.Filter = "hb|*.hb";
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 1000;
            this.ToolTip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            // 
            // drop_chapterFinish
            // 
            this.drop_chapterFinish.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_chapterFinish.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_chapterFinish.FormattingEnabled = true;
            this.drop_chapterFinish.Location = new System.Drawing.Point(397, 52);
            this.drop_chapterFinish.Name = "drop_chapterFinish";
            this.drop_chapterFinish.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterFinish.TabIndex = 10;
            this.drop_chapterFinish.Text = "Auto";
            this.ToolTip.SetToolTip(this.drop_chapterFinish, "Select the chapter range you would like to enocde. (default: All Chapters)");
            this.drop_chapterFinish.SelectedIndexChanged += new System.EventHandler(this.drop_chapterFinish_SelectedIndexChanged);
            // 
            // drop_chapterStart
            // 
            this.drop_chapterStart.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_chapterStart.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_chapterStart.FormattingEnabled = true;
            this.drop_chapterStart.Location = new System.Drawing.Point(295, 52);
            this.drop_chapterStart.Name = "drop_chapterStart";
            this.drop_chapterStart.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterStart.TabIndex = 9;
            this.drop_chapterStart.Text = "Auto";
            this.ToolTip.SetToolTip(this.drop_chapterStart, "Select the chapter range you would like to enocde. (default: All Chapters)");
            this.drop_chapterStart.SelectedIndexChanged += new System.EventHandler(this.drop_chapterStart_SelectedIndexChanged);
            // 
            // drp_dvdtitle
            // 
            this.drp_dvdtitle.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_dvdtitle.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_dvdtitle.FormattingEnabled = true;
            this.drp_dvdtitle.Items.AddRange(new object[] {
            "Automatic"});
            this.drp_dvdtitle.Location = new System.Drawing.Point(99, 52);
            this.drp_dvdtitle.Name = "drp_dvdtitle";
            this.drp_dvdtitle.Size = new System.Drawing.Size(119, 21);
            this.drp_dvdtitle.TabIndex = 7;
            this.drp_dvdtitle.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_dvdtitle, "Select the title you wish to encode.");
            this.drp_dvdtitle.SelectedIndexChanged += new System.EventHandler(this.drp_dvdtitle_SelectedIndexChanged);
            this.drp_dvdtitle.Click += new System.EventHandler(this.drp_dvdtitle_Click);
            // 
            // text_source
            // 
            this.text_source.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_source.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_source.Location = new System.Drawing.Point(99, 19);
            this.text_source.Name = "text_source";
            this.text_source.Size = new System.Drawing.Size(429, 21);
            this.text_source.TabIndex = 1;
            this.text_source.Text = "Click \'Browse\' to continue";
            this.ToolTip.SetToolTip(this.text_source, "The input source location.");
            // 
            // text_destination
            // 
            this.text_destination.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_destination.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_destination.Location = new System.Drawing.Point(99, 19);
            this.text_destination.Name = "text_destination";
            this.text_destination.Size = new System.Drawing.Size(493, 21);
            this.text_destination.TabIndex = 1;
            this.ToolTip.SetToolTip(this.text_destination, "Location where the encoded file will be saved.");
            this.text_destination.TextChanged += new System.EventHandler(this.text_destination_TextChanged);
            // 
            // drp_videoEncoder
            // 
            this.drp_videoEncoder.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_videoEncoder.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_videoEncoder.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_videoEncoder.FormattingEnabled = true;
            this.drp_videoEncoder.Items.AddRange(new object[] {
            "Mpeg 4",
            "Xvid",
            "H.264"});
            this.drp_videoEncoder.Location = new System.Drawing.Point(99, 20);
            this.drp_videoEncoder.Name = "drp_videoEncoder";
            this.drp_videoEncoder.Size = new System.Drawing.Size(156, 21);
            this.drp_videoEncoder.TabIndex = 1;
            this.ToolTip.SetToolTip(this.drp_videoEncoder, "Select a video encoder");
            this.drp_videoEncoder.SelectedIndexChanged += new System.EventHandler(this.drp_videoEncoder_SelectedIndexChanged);
            // 
            // drp_audbit_1
            // 
            this.drp_audbit_1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audbit_1.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audbit_1.FormattingEnabled = true;
            this.drp_audbit_1.Items.AddRange(new object[] {
            "32",
            "40",
            "48",
            "56",
            "64",
            "80",
            "86",
            "112",
            "128",
            "160"});
            this.drp_audbit_1.Location = new System.Drawing.Point(589, 49);
            this.drp_audbit_1.Name = "drp_audbit_1";
            this.drp_audbit_1.Size = new System.Drawing.Size(93, 20);
            this.drp_audbit_1.TabIndex = 9;
            this.drp_audbit_1.Text = "160";
            this.ToolTip.SetToolTip(this.drp_audbit_1, "Set the Audio Bit-Rate");
            // 
            // drp_audsr_1
            // 
            this.drp_audsr_1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audsr_1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audsr_1.FormattingEnabled = true;
            this.drp_audsr_1.Items.AddRange(new object[] {
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audsr_1.Location = new System.Drawing.Point(499, 49);
            this.drp_audsr_1.Name = "drp_audsr_1";
            this.drp_audsr_1.Size = new System.Drawing.Size(64, 21);
            this.drp_audsr_1.TabIndex = 11;
            this.drp_audsr_1.Text = "48";
            this.ToolTip.SetToolTip(this.drp_audsr_1, "Set the Audio Sample Rate");
            // 
            // check_largeFile
            // 
            this.check_largeFile.AutoSize = true;
            this.check_largeFile.BackColor = System.Drawing.Color.Transparent;
            this.check_largeFile.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_largeFile.Location = new System.Drawing.Point(261, 22);
            this.check_largeFile.Name = "check_largeFile";
            this.check_largeFile.Size = new System.Drawing.Size(82, 17);
            this.check_largeFile.TabIndex = 4;
            this.check_largeFile.Text = "64Bit MP4";
            this.ToolTip.SetToolTip(this.check_largeFile, "Allows creation of MP4 files greater than 4GB.\r\nWarning: Breaks iPod, Apple TV (V" +
                    "ersion 1 only) and PS3 compatibility.\r\nRequires .mp4/.m4v file extension");
            this.check_largeFile.UseVisualStyleBackColor = false;
            this.check_largeFile.CheckedChanged += new System.EventHandler(this.check_largeFile_CheckedChanged);
            // 
            // check_turbo
            // 
            this.check_turbo.AutoSize = true;
            this.check_turbo.BackColor = System.Drawing.Color.Transparent;
            this.check_turbo.Enabled = false;
            this.check_turbo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_turbo.Location = new System.Drawing.Point(37, 151);
            this.check_turbo.Name = "check_turbo";
            this.check_turbo.Size = new System.Drawing.Size(115, 17);
            this.check_turbo.TabIndex = 3;
            this.check_turbo.Text = "Turbo first Pass";
            this.ToolTip.SetToolTip(this.check_turbo, "Makes the first pass of a 2 pass encode faster.");
            this.check_turbo.UseVisualStyleBackColor = false;
            // 
            // drp_videoFramerate
            // 
            this.drp_videoFramerate.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_videoFramerate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_videoFramerate.FormattingEnabled = true;
            this.drp_videoFramerate.Items.AddRange(new object[] {
            "Same as source",
            "5",
            "10",
            "12",
            "15",
            "23.976",
            "24",
            "25",
            "29.97"});
            this.drp_videoFramerate.Location = new System.Drawing.Point(125, 35);
            this.drp_videoFramerate.Name = "drp_videoFramerate";
            this.drp_videoFramerate.Size = new System.Drawing.Size(126, 21);
            this.drp_videoFramerate.TabIndex = 7;
            this.drp_videoFramerate.Text = "Same as source";
            this.ToolTip.SetToolTip(this.drp_videoFramerate, "Can be left to automcatic in most cases.");
            // 
            // slider_videoQuality
            // 
            this.slider_videoQuality.Location = new System.Drawing.Point(435, 90);
            this.slider_videoQuality.Maximum = 100;
            this.slider_videoQuality.Name = "slider_videoQuality";
            this.slider_videoQuality.Size = new System.Drawing.Size(167, 42);
            this.slider_videoQuality.TabIndex = 14;
            this.slider_videoQuality.TickFrequency = 17;
            this.ToolTip.SetToolTip(this.slider_videoQuality, "Set the quality level of the video. (Around 70% is fine for most)");
            this.slider_videoQuality.Scroll += new System.EventHandler(this.slider_videoQuality_Scroll);
            // 
            // text_filesize
            // 
            this.text_filesize.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_filesize.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_filesize.Location = new System.Drawing.Point(446, 63);
            this.text_filesize.Name = "text_filesize";
            this.text_filesize.Size = new System.Drawing.Size(81, 21);
            this.text_filesize.TabIndex = 12;
            this.ToolTip.SetToolTip(this.text_filesize, "Set the file size you wish the encoded file to be.");
            this.text_filesize.TextChanged += new System.EventHandler(this.text_filesize_TextChanged);
            // 
            // text_bitrate
            // 
            this.text_bitrate.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_bitrate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_bitrate.Location = new System.Drawing.Point(446, 36);
            this.text_bitrate.Name = "text_bitrate";
            this.text_bitrate.Size = new System.Drawing.Size(81, 21);
            this.text_bitrate.TabIndex = 10;
            this.ToolTip.SetToolTip(this.text_bitrate, "Set the bitrate of the video");
            this.text_bitrate.TextChanged += new System.EventHandler(this.text_bitrate_TextChanged);
            // 
            // drp_subtitle
            // 
            this.drp_subtitle.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_subtitle.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_subtitle.FormattingEnabled = true;
            this.drp_subtitle.Items.AddRange(new object[] {
            "None",
            "Autoselect"});
            this.drp_subtitle.Location = new System.Drawing.Point(393, 221);
            this.drp_subtitle.Name = "drp_subtitle";
            this.drp_subtitle.Size = new System.Drawing.Size(138, 21);
            this.drp_subtitle.TabIndex = 27;
            this.drp_subtitle.Text = "None";
            this.ToolTip.SetToolTip(this.drp_subtitle, resources.GetString("drp_subtitle.ToolTip"));
            this.drp_subtitle.SelectedIndexChanged += new System.EventHandler(this.drp_subtitle_SelectedIndexChanged);
            // 
            // btn_setDefault
            // 
            this.btn_setDefault.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_setDefault.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_setDefault.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_setDefault.Location = new System.Drawing.Point(115, 502);
            this.btn_setDefault.Name = "btn_setDefault";
            this.btn_setDefault.Size = new System.Drawing.Size(72, 22);
            this.btn_setDefault.TabIndex = 1;
            this.btn_setDefault.TabStop = false;
            this.btn_setDefault.Text = "Set Default";
            this.ToolTip.SetToolTip(this.btn_setDefault, "Set current settings as program defaults.\r\nRequires option to be enabled in Tools" +
                    " > Options");
            this.btn_setDefault.UseVisualStyleBackColor = true;
            this.btn_setDefault.Click += new System.EventHandler(this.btn_setDefault_Click);
            // 
            // drp_audmix_1
            // 
            this.drp_audmix_1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audmix_1.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audmix_1.FormattingEnabled = true;
            this.drp_audmix_1.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audmix_1.Location = new System.Drawing.Point(353, 49);
            this.drp_audmix_1.Name = "drp_audmix_1";
            this.drp_audmix_1.Size = new System.Drawing.Size(129, 20);
            this.drp_audmix_1.TabIndex = 5;
            this.drp_audmix_1.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_audmix_1, "Please note: Some options require a 5.1 audio channel to be selected");
            this.drp_audmix_1.SelectedIndexChanged += new System.EventHandler(this.drp_audioMixDown_SelectedIndexChanged);
            // 
            // text_height
            // 
            this.text_height.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_height.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_height.ForeColor = System.Drawing.SystemColors.InfoText;
            this.text_height.Location = new System.Drawing.Point(504, 35);
            this.text_height.Name = "text_height";
            this.text_height.Size = new System.Drawing.Size(64, 21);
            this.text_height.TabIndex = 28;
            this.ToolTip.SetToolTip(this.text_height, "Video Resolution (Height)\r\nCan only be altered when Anamorphic is set to \"None\"");
            this.text_height.TextChanged += new System.EventHandler(this.text_height_TextChanged);
            // 
            // text_width
            // 
            this.text_width.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_width.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_width.Location = new System.Drawing.Point(413, 35);
            this.text_width.Name = "text_width";
            this.text_width.Size = new System.Drawing.Size(64, 21);
            this.text_width.TabIndex = 26;
            this.ToolTip.SetToolTip(this.text_width, "Video Resolution (Width)\r\nCan only be altered when Anamorphic is set to \"None\" or" +
                    " \"Loose\"");
            this.text_width.TextChanged += new System.EventHandler(this.text_width_TextChanged);
            // 
            // check_optimiseMP4
            // 
            this.check_optimiseMP4.AutoSize = true;
            this.check_optimiseMP4.BackColor = System.Drawing.Color.Transparent;
            this.check_optimiseMP4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_optimiseMP4.Location = new System.Drawing.Point(349, 22);
            this.check_optimiseMP4.Name = "check_optimiseMP4";
            this.check_optimiseMP4.Size = new System.Drawing.Size(143, 17);
            this.check_optimiseMP4.TabIndex = 25;
            this.check_optimiseMP4.Text = "HTTP Optimized MP4";
            this.ToolTip.SetToolTip(this.check_optimiseMP4, "MP4 files can be optimized for progressive downloads over the Web,\r\nbut note that" +
                    " QuickTime can only read the files as long as the file extension is .mp4\r\nCan on" +
                    "ly be used with H.264 ");
            this.check_optimiseMP4.UseVisualStyleBackColor = false;
            this.check_optimiseMP4.CheckedChanged += new System.EventHandler(this.check_optimiseMP4_CheckedChanged);
            // 
            // check_iPodAtom
            // 
            this.check_iPodAtom.AutoSize = true;
            this.check_iPodAtom.BackColor = System.Drawing.Color.Transparent;
            this.check_iPodAtom.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_iPodAtom.Location = new System.Drawing.Point(498, 23);
            this.check_iPodAtom.Name = "check_iPodAtom";
            this.check_iPodAtom.Size = new System.Drawing.Size(122, 17);
            this.check_iPodAtom.TabIndex = 26;
            this.check_iPodAtom.Text = "Insert iPod Atom";
            this.ToolTip.SetToolTip(this.check_iPodAtom, "Required for 5th and 6th Generation iPods.\r\nEncodes will not sync if this is not " +
                    "enabled for H.264 encodes");
            this.check_iPodAtom.UseVisualStyleBackColor = false;
            this.check_iPodAtom.CheckedChanged += new System.EventHandler(this.check_iPodAtom_CheckedChanged);
            // 
            // data_chpt
            // 
            this.data_chpt.AllowUserToAddRows = false;
            this.data_chpt.AllowUserToDeleteRows = false;
            this.data_chpt.AllowUserToResizeRows = false;
            this.data_chpt.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.data_chpt.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.number,
            this.name});
            this.data_chpt.Location = new System.Drawing.Point(16, 55);
            this.data_chpt.Name = "data_chpt";
            this.data_chpt.Size = new System.Drawing.Size(661, 234);
            this.data_chpt.TabIndex = 3;
            this.ToolTip.SetToolTip(this.data_chpt, resources.GetString("data_chpt.ToolTip"));
            // 
            // number
            // 
            dataGridViewCellStyle1.Format = "N0";
            dataGridViewCellStyle1.NullValue = null;
            this.number.DefaultCellStyle = dataGridViewCellStyle1;
            this.number.HeaderText = "Chapter Number";
            this.number.MaxInputLength = 3;
            this.number.Name = "number";
            this.number.Width = 135;
            // 
            // name
            // 
            this.name.HeaderText = "Chapter Name";
            this.name.Name = "name";
            this.name.Width = 460;
            // 
            // btn_addPreset
            // 
            this.btn_addPreset.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_addPreset.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_addPreset.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_addPreset.Location = new System.Drawing.Point(10, 502);
            this.btn_addPreset.Name = "btn_addPreset";
            this.btn_addPreset.Size = new System.Drawing.Size(35, 22);
            this.btn_addPreset.TabIndex = 3;
            this.btn_addPreset.TabStop = false;
            this.btn_addPreset.Text = "Add";
            this.ToolTip.SetToolTip(this.btn_addPreset, "Set current settings as program defaults.\r\nRequires option to be enabled in Tools" +
                    " > Options");
            this.btn_addPreset.UseVisualStyleBackColor = true;
            this.btn_addPreset.Click += new System.EventHandler(this.btn_addPreset_Click);
            // 
            // btn_removePreset
            // 
            this.btn_removePreset.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_removePreset.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_removePreset.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_removePreset.Location = new System.Drawing.Point(51, 502);
            this.btn_removePreset.Name = "btn_removePreset";
            this.btn_removePreset.Size = new System.Drawing.Size(58, 22);
            this.btn_removePreset.TabIndex = 4;
            this.btn_removePreset.TabStop = false;
            this.btn_removePreset.Text = "Remove";
            this.ToolTip.SetToolTip(this.btn_removePreset, "Set current settings as program defaults.\r\nRequires option to be enabled in Tools" +
                    " > Options");
            this.btn_removePreset.UseVisualStyleBackColor = true;
            this.btn_removePreset.Click += new System.EventHandler(this.btn_removePreset_Click);
            // 
            // drp_audmix_2
            // 
            this.drp_audmix_2.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audmix_2.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audmix_2.FormattingEnabled = true;
            this.drp_audmix_2.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audmix_2.Location = new System.Drawing.Point(353, 78);
            this.drp_audmix_2.Name = "drp_audmix_2";
            this.drp_audmix_2.Size = new System.Drawing.Size(129, 20);
            this.drp_audmix_2.TabIndex = 14;
            this.drp_audmix_2.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_audmix_2, "Please note: Some options require a 5.1 audio channel to be selected");
            this.drp_audmix_2.SelectedIndexChanged += new System.EventHandler(this.drp_audmix_2_SelectedIndexChanged);
            // 
            // drp_audenc_1
            // 
            this.drp_audenc_1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audenc_1.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audenc_1.FormattingEnabled = true;
            this.drp_audenc_1.Items.AddRange(new object[] {
            "AAC",
            "MP3",
            "Vorbis",
            "AC3"});
            this.drp_audenc_1.Location = new System.Drawing.Point(236, 49);
            this.drp_audenc_1.Name = "drp_audenc_1";
            this.drp_audenc_1.Size = new System.Drawing.Size(111, 20);
            this.drp_audenc_1.TabIndex = 7;
            this.drp_audenc_1.Text = "AAC";
            this.ToolTip.SetToolTip(this.drp_audenc_1, "Select an audio encoder.");
            this.drp_audenc_1.SelectedIndexChanged += new System.EventHandler(this.drp_audenc_1_SelectedIndexChanged);
            // 
            // drp_audenc_2
            // 
            this.drp_audenc_2.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audenc_2.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audenc_2.FormattingEnabled = true;
            this.drp_audenc_2.Items.AddRange(new object[] {
            "AAC",
            "MP3",
            "Vorbis",
            "AC3"});
            this.drp_audenc_2.Location = new System.Drawing.Point(236, 78);
            this.drp_audenc_2.Name = "drp_audenc_2";
            this.drp_audenc_2.Size = new System.Drawing.Size(111, 20);
            this.drp_audenc_2.TabIndex = 15;
            this.drp_audenc_2.Text = "AAC";
            this.ToolTip.SetToolTip(this.drp_audenc_2, "Select an audio encoder.");
            this.drp_audenc_2.SelectedIndexChanged += new System.EventHandler(this.drp_audenc_2_SelectedIndexChanged);
            // 
            // drp_audbit_2
            // 
            this.drp_audbit_2.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audbit_2.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audbit_2.FormattingEnabled = true;
            this.drp_audbit_2.Items.AddRange(new object[] {
            "32",
            "40",
            "48",
            "56",
            "64",
            "80",
            "86",
            "112",
            "128",
            "160"});
            this.drp_audbit_2.Location = new System.Drawing.Point(589, 78);
            this.drp_audbit_2.Name = "drp_audbit_2";
            this.drp_audbit_2.Size = new System.Drawing.Size(93, 20);
            this.drp_audbit_2.TabIndex = 16;
            this.drp_audbit_2.Text = "160";
            this.ToolTip.SetToolTip(this.drp_audbit_2, "Set the Audio Bit-Rate");
            // 
            // drp_audsr_2
            // 
            this.drp_audsr_2.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audsr_2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audsr_2.FormattingEnabled = true;
            this.drp_audsr_2.Items.AddRange(new object[] {
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audsr_2.Location = new System.Drawing.Point(499, 77);
            this.drp_audsr_2.Name = "drp_audsr_2";
            this.drp_audsr_2.Size = new System.Drawing.Size(64, 21);
            this.drp_audsr_2.TabIndex = 17;
            this.drp_audsr_2.Text = "48";
            this.ToolTip.SetToolTip(this.drp_audsr_2, "Set the Audio Sample Rate");
            // 
            // check_fileMode
            // 
            this.check_fileMode.AutoSize = true;
            this.check_fileMode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_fileMode.Location = new System.Drawing.Point(618, 21);
            this.check_fileMode.Name = "check_fileMode";
            this.check_fileMode.Size = new System.Drawing.Size(79, 17);
            this.check_fileMode.TabIndex = 11;
            this.check_fileMode.Text = "File Mode";
            this.ToolTip.SetToolTip(this.check_fileMode, "Displays a File selection dialog box instead of the standard folder selection.\r\nW" +
                    "ith the file dialog box, you can select the folllowing file types:\r\n.vob, .ts, ." +
                    "mpg, .mpeg and .m2t");
            this.check_fileMode.UseVisualStyleBackColor = true;
            // 
            // slider_drc
            // 
            this.slider_drc.LargeChange = 0;
            this.slider_drc.Location = new System.Drawing.Point(16, 239);
            this.slider_drc.Maximum = 30;
            this.slider_drc.Name = "slider_drc";
            this.slider_drc.Size = new System.Drawing.Size(241, 42);
            this.slider_drc.TabIndex = 19;
            this.slider_drc.TickFrequency = 10;
            this.slider_drc.Scroll += new System.EventHandler(this.slider_drc_Scroll);
            // 
            // DVD_Open
            // 
            this.DVD_Open.Description = "Select the \"VIDEO_TS\" folder from your DVD Drive.";
            // 
            // File_Open
            // 
            this.File_Open.DefaultExt = "hb";
            this.File_Open.Filter = "hb|*.hb";
            // 
            // ISO_Open
            // 
            this.ISO_Open.DefaultExt = "ISO";
            this.ISO_Open.Filter = "All Supported Files|*.iso;*.mpg;*.m2t;*.vob;*.ts;*.mpeg;*.mpeg;";
            this.ISO_Open.RestoreDirectory = true;
            this.ISO_Open.SupportMultiDottedExtensions = true;
            // 
            // FileToolStripMenuItem
            // 
            this.FileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_open,
            this.mnu_save,
            this.toolStripSeparator2,
            this.mnu_exit});
            this.FileToolStripMenuItem.Name = "FileToolStripMenuItem";
            this.FileToolStripMenuItem.Size = new System.Drawing.Size(38, 20);
            this.FileToolStripMenuItem.Text = "&File";
            // 
            // mnu_open
            // 
            this.mnu_open.Image = ((System.Drawing.Image)(resources.GetObject("mnu_open.Image")));
            this.mnu_open.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.mnu_open.Name = "mnu_open";
            this.mnu_open.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.mnu_open.Size = new System.Drawing.Size(210, 22);
            this.mnu_open.Text = "&Import Preset";
            this.mnu_open.Click += new System.EventHandler(this.mnu_open_Click);
            // 
            // mnu_save
            // 
            this.mnu_save.Image = ((System.Drawing.Image)(resources.GetObject("mnu_save.Image")));
            this.mnu_save.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.mnu_save.Name = "mnu_save";
            this.mnu_save.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            this.mnu_save.Size = new System.Drawing.Size(210, 22);
            this.mnu_save.Text = "&Save Preset";
            this.mnu_save.Visible = false;
            this.mnu_save.Click += new System.EventHandler(this.mnu_save_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(207, 6);
            // 
            // mnu_exit
            // 
            this.mnu_exit.Name = "mnu_exit";
            this.mnu_exit.Size = new System.Drawing.Size(210, 22);
            this.mnu_exit.Text = "E&xit";
            this.mnu_exit.Click += new System.EventHandler(this.mnu_exit_Click);
            // 
            // mnu_open3
            // 
            this.mnu_open3.Name = "mnu_open3";
            this.mnu_open3.Size = new System.Drawing.Size(32, 19);
            // 
            // ToolsToolStripMenuItem
            // 
            this.ToolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_encode,
            this.mnu_viewDVDdata,
            this.ToolStripSeparator5,
            this.mnu_options});
            this.ToolsToolStripMenuItem.Name = "ToolsToolStripMenuItem";
            this.ToolsToolStripMenuItem.Size = new System.Drawing.Size(49, 20);
            this.ToolsToolStripMenuItem.Text = "&Tools";
            // 
            // mnu_encode
            // 
            this.mnu_encode.Image = global::Handbrake.Properties.Resources.Queue_Small;
            this.mnu_encode.Name = "mnu_encode";
            this.mnu_encode.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Q)));
            this.mnu_encode.Size = new System.Drawing.Size(217, 22);
            this.mnu_encode.Text = "Show Queue";
            this.mnu_encode.Click += new System.EventHandler(this.mnu_encode_Click);
            // 
            // mnu_viewDVDdata
            // 
            this.mnu_viewDVDdata.Image = global::Handbrake.Properties.Resources.Movies_Small;
            this.mnu_viewDVDdata.Name = "mnu_viewDVDdata";
            this.mnu_viewDVDdata.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.D)));
            this.mnu_viewDVDdata.Size = new System.Drawing.Size(217, 22);
            this.mnu_viewDVDdata.Text = "View DVD data";
            this.mnu_viewDVDdata.Click += new System.EventHandler(this.mnu_viewDVDdata_Click);
            // 
            // ToolStripSeparator5
            // 
            this.ToolStripSeparator5.Name = "ToolStripSeparator5";
            this.ToolStripSeparator5.Size = new System.Drawing.Size(214, 6);
            // 
            // mnu_options
            // 
            this.mnu_options.Image = global::Handbrake.Properties.Resources.Pref_Small;
            this.mnu_options.Name = "mnu_options";
            this.mnu_options.Size = new System.Drawing.Size(217, 22);
            this.mnu_options.Text = "Options";
            this.mnu_options.Click += new System.EventHandler(this.mnu_options_Click);
            // 
            // PresetsToolStripMenuItem
            // 
            this.PresetsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_SelectDefault,
            this.toolStripSeparator7,
            this.mnu_presetReset});
            this.PresetsToolStripMenuItem.Name = "PresetsToolStripMenuItem";
            this.PresetsToolStripMenuItem.Size = new System.Drawing.Size(61, 20);
            this.PresetsToolStripMenuItem.Text = "&Presets";
            // 
            // mnu_SelectDefault
            // 
            this.mnu_SelectDefault.Name = "mnu_SelectDefault";
            this.mnu_SelectDefault.Size = new System.Drawing.Size(215, 22);
            this.mnu_SelectDefault.Text = "Select Default Preset";
            this.mnu_SelectDefault.ToolTipText = "Select HandBrake\'s default preset";
            this.mnu_SelectDefault.Click += new System.EventHandler(this.mnu_SelectDefault_Click);
            // 
            // toolStripSeparator7
            // 
            this.toolStripSeparator7.Name = "toolStripSeparator7";
            this.toolStripSeparator7.Size = new System.Drawing.Size(212, 6);
            // 
            // mnu_presetReset
            // 
            this.mnu_presetReset.Name = "mnu_presetReset";
            this.mnu_presetReset.Size = new System.Drawing.Size(215, 22);
            this.mnu_presetReset.Text = "Update Built-in Presets";
            this.mnu_presetReset.ToolTipText = "Resets all presets.";
            this.mnu_presetReset.Click += new System.EventHandler(this.mnu_presetReset_Click);
            // 
            // HelpToolStripMenuItem
            // 
            this.HelpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.OnlineDocumentationToolStripMenuItem,
            this.mnu_handbrake_home,
            this.ToolStripSeparator3,
            this.mnu_UpdateCheck,
            this.toolStripSeparator6,
            this.mnu_about});
            this.HelpToolStripMenuItem.Name = "HelpToolStripMenuItem";
            this.HelpToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.HelpToolStripMenuItem.Text = "&Help";
            // 
            // OnlineDocumentationToolStripMenuItem
            // 
            this.OnlineDocumentationToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_wiki,
            this.mnu_faq,
            this.mnu_onlineDocs});
            this.OnlineDocumentationToolStripMenuItem.Name = "OnlineDocumentationToolStripMenuItem";
            this.OnlineDocumentationToolStripMenuItem.Size = new System.Drawing.Size(213, 22);
            this.OnlineDocumentationToolStripMenuItem.Text = "Online Documentation";
            // 
            // mnu_wiki
            // 
            this.mnu_wiki.Name = "mnu_wiki";
            this.mnu_wiki.Size = new System.Drawing.Size(217, 22);
            this.mnu_wiki.Text = "Wiki / User Guides";
            this.mnu_wiki.Click += new System.EventHandler(this.mnu_wiki_Click);
            // 
            // mnu_faq
            // 
            this.mnu_faq.Name = "mnu_faq";
            this.mnu_faq.Size = new System.Drawing.Size(217, 22);
            this.mnu_faq.Text = "FAQ";
            this.mnu_faq.Click += new System.EventHandler(this.mnu_faq_Click);
            // 
            // mnu_onlineDocs
            // 
            this.mnu_onlineDocs.Name = "mnu_onlineDocs";
            this.mnu_onlineDocs.Size = new System.Drawing.Size(217, 22);
            this.mnu_onlineDocs.Text = "Full Documentation List";
            this.mnu_onlineDocs.Click += new System.EventHandler(this.mnu_onlineDocs_Click);
            // 
            // mnu_handbrake_home
            // 
            this.mnu_handbrake_home.Name = "mnu_handbrake_home";
            this.mnu_handbrake_home.Size = new System.Drawing.Size(213, 22);
            this.mnu_handbrake_home.Text = "HandBrake Homepage";
            this.mnu_handbrake_home.Click += new System.EventHandler(this.mnu_handbrake_home_Click);
            // 
            // ToolStripSeparator3
            // 
            this.ToolStripSeparator3.Name = "ToolStripSeparator3";
            this.ToolStripSeparator3.Size = new System.Drawing.Size(210, 6);
            // 
            // mnu_UpdateCheck
            // 
            this.mnu_UpdateCheck.Name = "mnu_UpdateCheck";
            this.mnu_UpdateCheck.Size = new System.Drawing.Size(213, 22);
            this.mnu_UpdateCheck.Text = "Check for Updates";
            this.mnu_UpdateCheck.Click += new System.EventHandler(this.mnu_UpdateCheck_Click);
            // 
            // toolStripSeparator6
            // 
            this.toolStripSeparator6.Name = "toolStripSeparator6";
            this.toolStripSeparator6.Size = new System.Drawing.Size(210, 6);
            // 
            // mnu_about
            // 
            this.mnu_about.Name = "mnu_about";
            this.mnu_about.Size = new System.Drawing.Size(213, 22);
            this.mnu_about.Text = "About...";
            this.mnu_about.Click += new System.EventHandler(this.mnu_about_Click);
            // 
            // frmMainMenu
            // 
            this.frmMainMenu.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.frmMainMenu.GripStyle = System.Windows.Forms.ToolStripGripStyle.Visible;
            this.frmMainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileToolStripMenuItem,
            this.ToolsToolStripMenuItem,
            this.PresetsToolStripMenuItem,
            this.HelpToolStripMenuItem});
            this.frmMainMenu.Location = new System.Drawing.Point(0, 0);
            this.frmMainMenu.Name = "frmMainMenu";
            this.frmMainMenu.Size = new System.Drawing.Size(931, 24);
            this.frmMainMenu.TabIndex = 0;
            this.frmMainMenu.Text = "MenuStrip1";
            // 
            // GroupBox1
            // 
            this.GroupBox1.Controls.Add(this.btn_Browse);
            this.GroupBox1.Controls.Add(this.check_fileMode);
            this.GroupBox1.Controls.Add(this.Label13);
            this.GroupBox1.Controls.Add(this.drop_chapterFinish);
            this.GroupBox1.Controls.Add(this.drop_chapterStart);
            this.GroupBox1.Controls.Add(this.drp_dvdtitle);
            this.GroupBox1.Controls.Add(this.Label17);
            this.GroupBox1.Controls.Add(this.text_source);
            this.GroupBox1.Controls.Add(this.Label9);
            this.GroupBox1.Controls.Add(this.Label10);
            this.GroupBox1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GroupBox1.ForeColor = System.Drawing.Color.Black;
            this.GroupBox1.Location = new System.Drawing.Point(14, 73);
            this.GroupBox1.Name = "GroupBox1";
            this.GroupBox1.Size = new System.Drawing.Size(705, 87);
            this.GroupBox1.TabIndex = 1;
            this.GroupBox1.TabStop = false;
            this.GroupBox1.Text = "Source";
            // 
            // btn_Browse
            // 
            this.btn_Browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Browse.Location = new System.Drawing.Point(534, 18);
            this.btn_Browse.Name = "btn_Browse";
            this.btn_Browse.Size = new System.Drawing.Size(75, 23);
            this.btn_Browse.TabIndex = 12;
            this.btn_Browse.Text = "Browse";
            this.btn_Browse.UseVisualStyleBackColor = true;
            this.btn_Browse.Click += new System.EventHandler(this.btn_Browse_Click);
            // 
            // Label13
            // 
            this.Label13.AutoSize = true;
            this.Label13.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label13.Location = new System.Drawing.Point(370, 55);
            this.Label13.Name = "Label13";
            this.Label13.Size = new System.Drawing.Size(21, 13);
            this.Label13.TabIndex = 10;
            this.Label13.Text = "To";
            // 
            // Label17
            // 
            this.Label17.AutoSize = true;
            this.Label17.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label17.ForeColor = System.Drawing.Color.Black;
            this.Label17.Location = new System.Drawing.Point(17, 22);
            this.Label17.Name = "Label17";
            this.Label17.Size = new System.Drawing.Size(52, 13);
            this.Label17.TabIndex = 0;
            this.Label17.Text = "Source:";
            // 
            // Label9
            // 
            this.Label9.AutoSize = true;
            this.Label9.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label9.ForeColor = System.Drawing.Color.Black;
            this.Label9.Location = new System.Drawing.Point(225, 55);
            this.Label9.Name = "Label9";
            this.Label9.Size = new System.Drawing.Size(64, 13);
            this.Label9.TabIndex = 8;
            this.Label9.Text = "Chapters:";
            // 
            // Label10
            // 
            this.Label10.AutoSize = true;
            this.Label10.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label10.ForeColor = System.Drawing.Color.Black;
            this.Label10.Location = new System.Drawing.Point(17, 55);
            this.Label10.Name = "Label10";
            this.Label10.Size = new System.Drawing.Size(36, 13);
            this.Label10.TabIndex = 6;
            this.Label10.Text = "Title:";
            // 
            // groupBox_output
            // 
            this.groupBox_output.Controls.Add(this.drp_videoEncoder);
            this.groupBox_output.Controls.Add(this.Label47);
            this.groupBox_output.Controls.Add(this.check_largeFile);
            this.groupBox_output.Controls.Add(this.check_iPodAtom);
            this.groupBox_output.Controls.Add(this.check_optimiseMP4);
            this.groupBox_output.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox_output.ForeColor = System.Drawing.Color.Black;
            this.groupBox_output.Location = new System.Drawing.Point(14, 217);
            this.groupBox_output.Name = "groupBox_output";
            this.groupBox_output.Size = new System.Drawing.Size(705, 50);
            this.groupBox_output.TabIndex = 3;
            this.groupBox_output.TabStop = false;
            this.groupBox_output.Text = "Output Settings (Preset: None)";
            // 
            // Label47
            // 
            this.Label47.AutoSize = true;
            this.Label47.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label47.ForeColor = System.Drawing.Color.Black;
            this.Label47.Location = new System.Drawing.Point(17, 24);
            this.Label47.Name = "Label47";
            this.Label47.Size = new System.Drawing.Size(62, 13);
            this.Label47.TabIndex = 0;
            this.Label47.Text = "Encoder: ";
            // 
            // Label3
            // 
            this.Label3.AutoSize = true;
            this.Label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label3.ForeColor = System.Drawing.Color.Black;
            this.Label3.Location = new System.Drawing.Point(17, 21);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(31, 13);
            this.Label3.TabIndex = 0;
            this.Label3.Text = "File:";
            // 
            // lbl_encode
            // 
            this.lbl_encode.AutoSize = true;
            this.lbl_encode.BackColor = System.Drawing.Color.Transparent;
            this.lbl_encode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_encode.Location = new System.Drawing.Point(448, 36);
            this.lbl_encode.Name = "lbl_encode";
            this.lbl_encode.Size = new System.Drawing.Size(129, 13);
            this.lbl_encode.TabIndex = 5;
            this.lbl_encode.Text = "Encoding started...";
            this.lbl_encode.Visible = false;
            // 
            // TabPage2
            // 
            this.TabPage2.BackColor = System.Drawing.Color.Transparent;
            this.TabPage2.Controls.Add(this.groupBox5);
            this.TabPage2.Controls.Add(this.groupBox4);
            this.TabPage2.Controls.Add(this.groupBox3);
            this.TabPage2.Controls.Add(this.label12);
            this.TabPage2.Controls.Add(this.label68);
            this.TabPage2.Controls.Add(this.label67);
            this.TabPage2.Controls.Add(this.drp_audsr_2);
            this.TabPage2.Controls.Add(this.label66);
            this.TabPage2.Controls.Add(this.drp_audbit_2);
            this.TabPage2.Controls.Add(this.label65);
            this.TabPage2.Controls.Add(this.label14);
            this.TabPage2.Controls.Add(this.drp_audenc_2);
            this.TabPage2.Controls.Add(this.drp_audenc_1);
            this.TabPage2.Controls.Add(this.drp_audmix_2);
            this.TabPage2.Controls.Add(this.label30);
            this.TabPage2.Controls.Add(this.label29);
            this.TabPage2.Controls.Add(this.label23);
            this.TabPage2.Controls.Add(this.label7);
            this.TabPage2.Controls.Add(this.lbl_drc);
            this.TabPage2.Controls.Add(this.slider_drc);
            this.TabPage2.Controls.Add(this.check_forced);
            this.TabPage2.Controls.Add(this.drp_track2Audio);
            this.TabPage2.Controls.Add(this.label28);
            this.TabPage2.Controls.Add(this.label27);
            this.TabPage2.Controls.Add(this.drp_subtitle);
            this.TabPage2.Controls.Add(this.Label19);
            this.TabPage2.Controls.Add(this.Label20);
            this.TabPage2.Controls.Add(this.drp_audmix_1);
            this.TabPage2.Controls.Add(this.drp_track1Audio);
            this.TabPage2.Controls.Add(this.drp_audbit_1);
            this.TabPage2.Controls.Add(this.Label5);
            this.TabPage2.Controls.Add(this.Label32);
            this.TabPage2.Controls.Add(this.drp_audsr_1);
            this.TabPage2.Location = new System.Drawing.Point(4, 22);
            this.TabPage2.Name = "TabPage2";
            this.TabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage2.Size = new System.Drawing.Size(697, 302);
            this.TabPage2.TabIndex = 3;
            this.TabPage2.Text = "Audio && Subtitles";
            // 
            // groupBox5
            // 
            this.groupBox5.Location = new System.Drawing.Point(107, 13);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(577, 10);
            this.groupBox5.TabIndex = 32;
            this.groupBox5.TabStop = false;
            // 
            // groupBox4
            // 
            this.groupBox4.Location = new System.Drawing.Point(109, 200);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(178, 10);
            this.groupBox4.TabIndex = 31;
            this.groupBox4.TabStop = false;
            // 
            // groupBox3
            // 
            this.groupBox3.Location = new System.Drawing.Point(397, 200);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(287, 10);
            this.groupBox3.TabIndex = 30;
            this.groupBox3.TabStop = false;
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.BackColor = System.Drawing.Color.Transparent;
            this.label12.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label12.Location = new System.Drawing.Point(19, 221);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(181, 13);
            this.label12.TabIndex = 29;
            this.label12.Text = "Dynamic Range Compression:";
            // 
            // label68
            // 
            this.label68.AutoSize = true;
            this.label68.BackColor = System.Drawing.Color.Transparent;
            this.label68.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label68.Location = new System.Drawing.Point(13, 13);
            this.label68.Name = "label68";
            this.label68.Size = new System.Drawing.Size(92, 13);
            this.label68.TabIndex = 0;
            this.label68.Text = "Audio Tracks";
            // 
            // label67
            // 
            this.label67.AutoSize = true;
            this.label67.BackColor = System.Drawing.Color.Transparent;
            this.label67.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label67.Location = new System.Drawing.Point(487, 32);
            this.label67.Name = "label67";
            this.label67.Size = new System.Drawing.Size(99, 12);
            this.label67.TabIndex = 10;
            this.label67.Text = "Sample Rate (kHz)";
            // 
            // label66
            // 
            this.label66.AutoSize = true;
            this.label66.BackColor = System.Drawing.Color.Transparent;
            this.label66.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label66.Location = new System.Drawing.Point(594, 32);
            this.label66.Name = "label66";
            this.label66.Size = new System.Drawing.Size(78, 12);
            this.label66.TabIndex = 8;
            this.label66.Text = "Bitrate (Kbps)";
            // 
            // label65
            // 
            this.label65.AutoSize = true;
            this.label65.BackColor = System.Drawing.Color.Transparent;
            this.label65.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label65.Location = new System.Drawing.Point(255, 32);
            this.label65.Name = "label65";
            this.label65.Size = new System.Drawing.Size(66, 12);
            this.label65.TabIndex = 6;
            this.label65.Text = "Audio Codec";
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.BackColor = System.Drawing.Color.Transparent;
            this.label14.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label14.Location = new System.Drawing.Point(380, 32);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(49, 12);
            this.label14.TabIndex = 4;
            this.label14.Text = "Mixdown";
            // 
            // label30
            // 
            this.label30.AutoSize = true;
            this.label30.BackColor = System.Drawing.Color.Transparent;
            this.label30.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label30.Location = new System.Drawing.Point(139, 271);
            this.label30.Name = "label30";
            this.label30.Size = new System.Drawing.Size(67, 12);
            this.label30.TabIndex = 23;
            this.label30.Text = "3.0 (Louder)";
            // 
            // label29
            // 
            this.label29.AutoSize = true;
            this.label29.BackColor = System.Drawing.Color.Transparent;
            this.label29.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label29.Location = new System.Drawing.Point(74, 271);
            this.label29.Name = "label29";
            this.label29.Size = new System.Drawing.Size(57, 12);
            this.label29.TabIndex = 22;
            this.label29.Text = "2.0 (Loud)";
            // 
            // label23
            // 
            this.label23.AutoSize = true;
            this.label23.BackColor = System.Drawing.Color.Transparent;
            this.label23.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label23.Location = new System.Drawing.Point(211, 271);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(73, 12);
            this.label23.TabIndex = 24;
            this.label23.Text = "4.0 (Loudest)";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.BackColor = System.Drawing.Color.Transparent;
            this.label7.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label7.Location = new System.Drawing.Point(20, 271);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(48, 12);
            this.label7.TabIndex = 21;
            this.label7.Text = "1.0 (Off)";
            // 
            // lbl_drc
            // 
            this.lbl_drc.AutoSize = true;
            this.lbl_drc.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drc.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_drc.Location = new System.Drawing.Point(206, 221);
            this.lbl_drc.Name = "lbl_drc";
            this.lbl_drc.Size = new System.Drawing.Size(14, 13);
            this.lbl_drc.TabIndex = 20;
            this.lbl_drc.Text = "1";
            // 
            // check_forced
            // 
            this.check_forced.AutoSize = true;
            this.check_forced.BackColor = System.Drawing.Color.Transparent;
            this.check_forced.Enabled = false;
            this.check_forced.Location = new System.Drawing.Point(537, 224);
            this.check_forced.Name = "check_forced";
            this.check_forced.Size = new System.Drawing.Size(147, 17);
            this.check_forced.TabIndex = 28;
            this.check_forced.Text = "Forced Subtitles Only";
            this.check_forced.UseVisualStyleBackColor = false;
            // 
            // drp_track2Audio
            // 
            this.drp_track2Audio.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_track2Audio.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_track2Audio.FormattingEnabled = true;
            this.drp_track2Audio.Items.AddRange(new object[] {
            "None",
            "Automatic"});
            this.drp_track2Audio.Location = new System.Drawing.Point(36, 77);
            this.drp_track2Audio.Name = "drp_track2Audio";
            this.drp_track2Audio.Size = new System.Drawing.Size(194, 20);
            this.drp_track2Audio.TabIndex = 13;
            this.drp_track2Audio.SelectedIndexChanged += new System.EventHandler(this.drp_track2Audio_SelectedIndexChanged);
            // 
            // label28
            // 
            this.label28.AutoSize = true;
            this.label28.BackColor = System.Drawing.Color.Transparent;
            this.label28.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label28.Location = new System.Drawing.Point(13, 80);
            this.label28.Name = "label28";
            this.label28.Size = new System.Drawing.Size(19, 13);
            this.label28.TabIndex = 12;
            this.label28.Text = "2:";
            // 
            // label27
            // 
            this.label27.AutoSize = true;
            this.label27.BackColor = System.Drawing.Color.Transparent;
            this.label27.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label27.Location = new System.Drawing.Point(107, 32);
            this.label27.Name = "label27";
            this.label27.Size = new System.Drawing.Size(38, 12);
            this.label27.TabIndex = 1;
            this.label27.Text = "Source";
            // 
            // Label19
            // 
            this.Label19.AutoSize = true;
            this.Label19.BackColor = System.Drawing.Color.Transparent;
            this.Label19.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label19.Location = new System.Drawing.Point(327, 200);
            this.Label19.Name = "Label19";
            this.Label19.Size = new System.Drawing.Size(64, 13);
            this.Label19.TabIndex = 25;
            this.Label19.Text = "Subtitles";
            // 
            // Label20
            // 
            this.Label20.AutoSize = true;
            this.Label20.BackColor = System.Drawing.Color.Transparent;
            this.Label20.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label20.Location = new System.Drawing.Point(327, 224);
            this.Label20.Name = "Label20";
            this.Label20.Size = new System.Drawing.Size(61, 13);
            this.Label20.TabIndex = 26;
            this.Label20.Text = "Subtitles:";
            // 
            // drp_track1Audio
            // 
            this.drp_track1Audio.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_track1Audio.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_track1Audio.FormattingEnabled = true;
            this.drp_track1Audio.Items.AddRange(new object[] {
            "Automatic",
            "None"});
            this.drp_track1Audio.Location = new System.Drawing.Point(36, 50);
            this.drp_track1Audio.Name = "drp_track1Audio";
            this.drp_track1Audio.Size = new System.Drawing.Size(194, 20);
            this.drp_track1Audio.TabIndex = 3;
            this.drp_track1Audio.SelectedIndexChanged += new System.EventHandler(this.drp_track1Audio_SelectedIndexChanged);
            // 
            // Label5
            // 
            this.Label5.AutoSize = true;
            this.Label5.BackColor = System.Drawing.Color.Transparent;
            this.Label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label5.Location = new System.Drawing.Point(13, 200);
            this.Label5.Name = "Label5";
            this.Label5.Size = new System.Drawing.Size(93, 13);
            this.Label5.TabIndex = 18;
            this.Label5.Text = "Audio Effects";
            // 
            // Label32
            // 
            this.Label32.AutoSize = true;
            this.Label32.BackColor = System.Drawing.Color.Transparent;
            this.Label32.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label32.Location = new System.Drawing.Point(13, 53);
            this.Label32.Name = "Label32";
            this.Label32.Size = new System.Drawing.Size(19, 13);
            this.Label32.TabIndex = 2;
            this.Label32.Text = "1:";
            // 
            // TabPage3
            // 
            this.TabPage3.BackColor = System.Drawing.Color.Transparent;
            this.TabPage3.Controls.Add(this.label25);
            this.TabPage3.Controls.Add(this.lbl_vfr);
            this.TabPage3.Controls.Add(this.check_grayscale);
            this.TabPage3.Controls.Add(this.check_turbo);
            this.TabPage3.Controls.Add(this.Label22);
            this.TabPage3.Controls.Add(this.check_2PassEncode);
            this.TabPage3.Controls.Add(this.Label2);
            this.TabPage3.Controls.Add(this.text_filesize);
            this.TabPage3.Controls.Add(this.Label42);
            this.TabPage3.Controls.Add(this.slider_videoQuality);
            this.TabPage3.Controls.Add(this.text_bitrate);
            this.TabPage3.Controls.Add(Label38);
            this.TabPage3.Controls.Add(this.SliderValue);
            this.TabPage3.Controls.Add(this.Label46);
            this.TabPage3.Controls.Add(this.Label40);
            this.TabPage3.Controls.Add(this.drp_videoFramerate);
            this.TabPage3.Location = new System.Drawing.Point(4, 22);
            this.TabPage3.Name = "TabPage3";
            this.TabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage3.Size = new System.Drawing.Size(697, 302);
            this.TabPage3.TabIndex = 2;
            this.TabPage3.Text = "Video";
            // 
            // label25
            // 
            this.label25.AutoSize = true;
            this.label25.BackColor = System.Drawing.Color.Transparent;
            this.label25.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label25.Location = new System.Drawing.Point(13, 13);
            this.label25.Name = "label25";
            this.label25.Size = new System.Drawing.Size(76, 13);
            this.label25.TabIndex = 28;
            this.label25.Text = "Framerate";
            // 
            // lbl_vfr
            // 
            this.lbl_vfr.AutoSize = true;
            this.lbl_vfr.BackColor = System.Drawing.Color.Transparent;
            this.lbl_vfr.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_vfr.Location = new System.Drawing.Point(123, 64);
            this.lbl_vfr.Name = "lbl_vfr";
            this.lbl_vfr.Size = new System.Drawing.Size(52, 12);
            this.lbl_vfr.TabIndex = 27;
            this.lbl_vfr.Text = "(VFR On)";
            this.lbl_vfr.Visible = false;
            // 
            // check_grayscale
            // 
            this.check_grayscale.AutoSize = true;
            this.check_grayscale.BackColor = System.Drawing.Color.Transparent;
            this.check_grayscale.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_grayscale.Location = new System.Drawing.Point(16, 105);
            this.check_grayscale.Name = "check_grayscale";
            this.check_grayscale.Size = new System.Drawing.Size(138, 17);
            this.check_grayscale.TabIndex = 1;
            this.check_grayscale.Text = "Grayscale Encoding";
            this.check_grayscale.UseVisualStyleBackColor = false;
            // 
            // Label22
            // 
            this.Label22.AutoSize = true;
            this.Label22.BackColor = System.Drawing.Color.Transparent;
            this.Label22.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label22.Location = new System.Drawing.Point(13, 85);
            this.Label22.Name = "Label22";
            this.Label22.Size = new System.Drawing.Size(191, 13);
            this.Label22.TabIndex = 0;
            this.Label22.Text = "Advanced Encoding Settings";
            // 
            // check_2PassEncode
            // 
            this.check_2PassEncode.AutoSize = true;
            this.check_2PassEncode.BackColor = System.Drawing.Color.Transparent;
            this.check_2PassEncode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_2PassEncode.Location = new System.Drawing.Point(16, 128);
            this.check_2PassEncode.Name = "check_2PassEncode";
            this.check_2PassEncode.Size = new System.Drawing.Size(119, 17);
            this.check_2PassEncode.TabIndex = 2;
            this.check_2PassEncode.Text = "2-Pass Encoding";
            this.check_2PassEncode.UseVisualStyleBackColor = false;
            this.check_2PassEncode.CheckedChanged += new System.EventHandler(this.check_2PassEncode_CheckedChanged);
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.BackColor = System.Drawing.Color.Transparent;
            this.Label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(304, 13);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(53, 13);
            this.Label2.TabIndex = 8;
            this.Label2.Text = "Quality";
            // 
            // Label42
            // 
            this.Label42.AutoSize = true;
            this.Label42.BackColor = System.Drawing.Color.Transparent;
            this.Label42.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label42.Location = new System.Drawing.Point(304, 38);
            this.Label42.Name = "Label42";
            this.Label42.Size = new System.Drawing.Size(117, 13);
            this.Label42.TabIndex = 9;
            this.Label42.Text = "Avg Bitrate (kbps):";
            // 
            // SliderValue
            // 
            this.SliderValue.AutoSize = true;
            this.SliderValue.BackColor = System.Drawing.Color.Transparent;
            this.SliderValue.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.SliderValue.Location = new System.Drawing.Point(599, 96);
            this.SliderValue.Name = "SliderValue";
            this.SliderValue.Size = new System.Drawing.Size(23, 12);
            this.SliderValue.TabIndex = 15;
            this.SliderValue.Text = "0%";
            // 
            // Label46
            // 
            this.Label46.AutoSize = true;
            this.Label46.BackColor = System.Drawing.Color.Transparent;
            this.Label46.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label46.Location = new System.Drawing.Point(13, 38);
            this.Label46.Name = "Label46";
            this.Label46.Size = new System.Drawing.Size(106, 13);
            this.Label46.TabIndex = 6;
            this.Label46.Text = "Framerate (FPS):";
            // 
            // Label40
            // 
            this.Label40.AutoSize = true;
            this.Label40.BackColor = System.Drawing.Color.Transparent;
            this.Label40.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label40.Location = new System.Drawing.Point(304, 95);
            this.Label40.Name = "Label40";
            this.Label40.Size = new System.Drawing.Size(107, 13);
            this.Label40.TabIndex = 13;
            this.Label40.Text = "Constant Quality:";
            // 
            // TabPage1
            // 
            this.TabPage1.BackColor = System.Drawing.Color.Transparent;
            this.TabPage1.Controls.Add(this.label6);
            this.TabPage1.Controls.Add(this.drp_anamorphic);
            this.TabPage1.Controls.Add(this.text_bottom);
            this.TabPage1.Controls.Add(this.text_top);
            this.TabPage1.Controls.Add(this.text_left);
            this.TabPage1.Controls.Add(this.text_right);
            this.TabPage1.Controls.Add(this.label26);
            this.TabPage1.Controls.Add(this.Label56);
            this.TabPage1.Controls.Add(this.lbl_Aspect);
            this.TabPage1.Controls.Add(this.Label91);
            this.TabPage1.Controls.Add(this.text_height);
            this.TabPage1.Controls.Add(this.Label55);
            this.TabPage1.Controls.Add(this.text_width);
            this.TabPage1.Controls.Add(this.check_vfr);
            this.TabPage1.Controls.Add(this.label24);
            this.TabPage1.Controls.Add(this.drp_deNoise);
            this.TabPage1.Controls.Add(this.label11);
            this.TabPage1.Controls.Add(this.check_deblock);
            this.TabPage1.Controls.Add(this.check_detelecine);
            this.TabPage1.Controls.Add(this.label4);
            this.TabPage1.Controls.Add(this.drp_deInterlace_option);
            this.TabPage1.Controls.Add(this.lbl_RecomendedCrop);
            this.TabPage1.Controls.Add(this.Label8);
            this.TabPage1.Controls.Add(this.Label1);
            this.TabPage1.Controls.Add(this.Label53);
            this.TabPage1.Controls.Add(this.Label52);
            this.TabPage1.Controls.Add(this.Label51);
            this.TabPage1.Controls.Add(this.Label50);
            this.TabPage1.Controls.Add(this.Label15);
            this.TabPage1.Controls.Add(this.drp_crop);
            this.TabPage1.Location = new System.Drawing.Point(4, 22);
            this.TabPage1.Name = "TabPage1";
            this.TabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage1.Size = new System.Drawing.Size(697, 302);
            this.TabPage1.TabIndex = 0;
            this.TabPage1.Text = "Picture Settings";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.BackColor = System.Drawing.Color.Transparent;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(311, 89);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(80, 13);
            this.label6.TabIndex = 37;
            this.label6.Text = "Anamorphic:";
            // 
            // drp_anamorphic
            // 
            this.drp_anamorphic.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_anamorphic.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_anamorphic.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_anamorphic.FormattingEnabled = true;
            this.drp_anamorphic.Items.AddRange(new object[] {
            "None",
            "Strict",
            "Loose"});
            this.drp_anamorphic.Location = new System.Drawing.Point(414, 86);
            this.drp_anamorphic.Name = "drp_anamorphic";
            this.drp_anamorphic.Size = new System.Drawing.Size(110, 21);
            this.drp_anamorphic.TabIndex = 36;
            this.drp_anamorphic.SelectedIndexChanged += new System.EventHandler(this.drp_anamorphic_SelectedIndexChanged);
            // 
            // text_bottom
            // 
            this.text_bottom.Location = new System.Drawing.Point(133, 146);
            this.text_bottom.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.text_bottom.Name = "text_bottom";
            this.text_bottom.Size = new System.Drawing.Size(44, 21);
            this.text_bottom.TabIndex = 35;
            // 
            // text_top
            // 
            this.text_top.Location = new System.Drawing.Point(133, 105);
            this.text_top.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.text_top.Name = "text_top";
            this.text_top.Size = new System.Drawing.Size(44, 21);
            this.text_top.TabIndex = 34;
            // 
            // text_left
            // 
            this.text_left.Location = new System.Drawing.Point(77, 126);
            this.text_left.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.text_left.Name = "text_left";
            this.text_left.Size = new System.Drawing.Size(44, 21);
            this.text_left.TabIndex = 33;
            // 
            // text_right
            // 
            this.text_right.Location = new System.Drawing.Point(187, 126);
            this.text_right.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.text_right.Name = "text_right";
            this.text_right.Size = new System.Drawing.Size(44, 21);
            this.text_right.TabIndex = 32;
            // 
            // label26
            // 
            this.label26.AutoSize = true;
            this.label26.BackColor = System.Drawing.Color.Transparent;
            this.label26.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label26.Location = new System.Drawing.Point(311, 13);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(34, 13);
            this.label26.TabIndex = 31;
            this.label26.Text = "Size";
            // 
            // Label56
            // 
            this.Label56.AutoSize = true;
            this.Label56.BackColor = System.Drawing.Color.Transparent;
            this.Label56.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label56.ForeColor = System.Drawing.Color.Black;
            this.Label56.Location = new System.Drawing.Point(483, 38);
            this.Label56.Name = "Label56";
            this.Label56.Size = new System.Drawing.Size(15, 13);
            this.Label56.TabIndex = 27;
            this.Label56.Text = "x";
            // 
            // lbl_Aspect
            // 
            this.lbl_Aspect.AutoSize = true;
            this.lbl_Aspect.BackColor = System.Drawing.Color.Transparent;
            this.lbl_Aspect.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_Aspect.Location = new System.Drawing.Point(412, 62);
            this.lbl_Aspect.Name = "lbl_Aspect";
            this.lbl_Aspect.Size = new System.Drawing.Size(72, 12);
            this.lbl_Aspect.TabIndex = 30;
            this.lbl_Aspect.Text = "Select a Title";
            // 
            // Label91
            // 
            this.Label91.AutoSize = true;
            this.Label91.BackColor = System.Drawing.Color.Transparent;
            this.Label91.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label91.Location = new System.Drawing.Point(311, 61);
            this.Label91.Name = "Label91";
            this.Label91.Size = new System.Drawing.Size(83, 13);
            this.Label91.TabIndex = 29;
            this.Label91.Text = "Aspect Ratio:";
            // 
            // Label55
            // 
            this.Label55.AutoSize = true;
            this.Label55.BackColor = System.Drawing.Color.Transparent;
            this.Label55.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label55.ForeColor = System.Drawing.Color.Black;
            this.Label55.Location = new System.Drawing.Point(311, 37);
            this.Label55.Name = "Label55";
            this.Label55.Size = new System.Drawing.Size(85, 13);
            this.Label55.TabIndex = 25;
            this.Label55.Text = "Width/Height:";
            // 
            // check_vfr
            // 
            this.check_vfr.AutoSize = true;
            this.check_vfr.BackColor = System.Drawing.Color.Transparent;
            this.check_vfr.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_vfr.Location = new System.Drawing.Point(413, 143);
            this.check_vfr.Name = "check_vfr";
            this.check_vfr.Size = new System.Drawing.Size(48, 17);
            this.check_vfr.TabIndex = 23;
            this.check_vfr.Text = "VFR";
            this.check_vfr.UseVisualStyleBackColor = false;
            this.check_vfr.CheckedChanged += new System.EventHandler(this.check_vfr_CheckedChanged);
            // 
            // label24
            // 
            this.label24.AutoSize = true;
            this.label24.BackColor = System.Drawing.Color.Transparent;
            this.label24.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label24.Location = new System.Drawing.Point(311, 120);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(49, 13);
            this.label24.TabIndex = 13;
            this.label24.Text = "Filters";
            // 
            // drp_deNoise
            // 
            this.drp_deNoise.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_deNoise.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_deNoise.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_deNoise.FormattingEnabled = true;
            this.drp_deNoise.Items.AddRange(new object[] {
            "None",
            "Weak",
            "Medium",
            "Strong"});
            this.drp_deNoise.Location = new System.Drawing.Point(413, 218);
            this.drp_deNoise.Name = "drp_deNoise";
            this.drp_deNoise.Size = new System.Drawing.Size(161, 21);
            this.drp_deNoise.TabIndex = 19;
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.BackColor = System.Drawing.Color.Transparent;
            this.label11.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(311, 221);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(58, 13);
            this.label11.TabIndex = 18;
            this.label11.Text = "Denoise:";
            // 
            // check_deblock
            // 
            this.check_deblock.AutoSize = true;
            this.check_deblock.BackColor = System.Drawing.Color.Transparent;
            this.check_deblock.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_deblock.Location = new System.Drawing.Point(314, 166);
            this.check_deblock.Name = "check_deblock";
            this.check_deblock.Size = new System.Drawing.Size(72, 17);
            this.check_deblock.TabIndex = 15;
            this.check_deblock.Text = "Deblock";
            this.check_deblock.UseVisualStyleBackColor = false;
            // 
            // check_detelecine
            // 
            this.check_detelecine.AutoSize = true;
            this.check_detelecine.BackColor = System.Drawing.Color.Transparent;
            this.check_detelecine.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_detelecine.Location = new System.Drawing.Point(314, 143);
            this.check_detelecine.Name = "check_detelecine";
            this.check_detelecine.Size = new System.Drawing.Size(86, 17);
            this.check_detelecine.TabIndex = 14;
            this.check_detelecine.Text = "Detelecine";
            this.check_detelecine.UseVisualStyleBackColor = false;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.BackColor = System.Drawing.Color.Transparent;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(311, 193);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(77, 13);
            this.label4.TabIndex = 16;
            this.label4.Text = "Deinterlace:";
            // 
            // drp_deInterlace_option
            // 
            this.drp_deInterlace_option.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_deInterlace_option.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_deInterlace_option.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_deInterlace_option.FormattingEnabled = true;
            this.drp_deInterlace_option.Items.AddRange(new object[] {
            "None",
            "Fast",
            "Slow",
            "Slower"});
            this.drp_deInterlace_option.Location = new System.Drawing.Point(413, 190);
            this.drp_deInterlace_option.Name = "drp_deInterlace_option";
            this.drp_deInterlace_option.Size = new System.Drawing.Size(161, 21);
            this.drp_deInterlace_option.TabIndex = 17;
            // 
            // lbl_RecomendedCrop
            // 
            this.lbl_RecomendedCrop.AutoSize = true;
            this.lbl_RecomendedCrop.BackColor = System.Drawing.Color.Transparent;
            this.lbl_RecomendedCrop.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_RecomendedCrop.Location = new System.Drawing.Point(116, 69);
            this.lbl_RecomendedCrop.Name = "lbl_RecomendedCrop";
            this.lbl_RecomendedCrop.Size = new System.Drawing.Size(72, 12);
            this.lbl_RecomendedCrop.TabIndex = 4;
            this.lbl_RecomendedCrop.Text = "Select a Title";
            // 
            // Label8
            // 
            this.Label8.AutoSize = true;
            this.Label8.BackColor = System.Drawing.Color.Transparent;
            this.Label8.Location = new System.Drawing.Point(13, 68);
            this.Label8.Name = "Label8";
            this.Label8.Size = new System.Drawing.Size(70, 13);
            this.Label8.TabIndex = 2;
            this.Label8.Text = "Auto Crop:";
            // 
            // Label1
            // 
            this.Label1.AutoSize = true;
            this.Label1.BackColor = System.Drawing.Color.Transparent;
            this.Label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label1.Location = new System.Drawing.Point(13, 13);
            this.Label1.Name = "Label1";
            this.Label1.Size = new System.Drawing.Size(37, 13);
            this.Label1.TabIndex = 0;
            this.Label1.Text = "Crop";
            // 
            // Label53
            // 
            this.Label53.AutoSize = true;
            this.Label53.BackColor = System.Drawing.Color.Transparent;
            this.Label53.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label53.Location = new System.Drawing.Point(130, 174);
            this.Label53.Name = "Label53";
            this.Label53.Size = new System.Drawing.Size(48, 13);
            this.Label53.TabIndex = 10;
            this.Label53.Text = "Bottom";
            // 
            // Label52
            // 
            this.Label52.AutoSize = true;
            this.Label52.BackColor = System.Drawing.Color.Transparent;
            this.Label52.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label52.Location = new System.Drawing.Point(139, 89);
            this.Label52.Name = "Label52";
            this.Label52.Size = new System.Drawing.Size(28, 13);
            this.Label52.TabIndex = 5;
            this.Label52.Text = "Top";
            // 
            // Label51
            // 
            this.Label51.AutoSize = true;
            this.Label51.BackColor = System.Drawing.Color.Transparent;
            this.Label51.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label51.Location = new System.Drawing.Point(237, 128);
            this.Label51.Name = "Label51";
            this.Label51.Size = new System.Drawing.Size(36, 13);
            this.Label51.TabIndex = 12;
            this.Label51.Text = "Right";
            // 
            // Label50
            // 
            this.Label50.AutoSize = true;
            this.Label50.BackColor = System.Drawing.Color.Transparent;
            this.Label50.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label50.Location = new System.Drawing.Point(13, 37);
            this.Label50.Name = "Label50";
            this.Label50.Size = new System.Drawing.Size(88, 13);
            this.Label50.TabIndex = 1;
            this.Label50.Text = "Select Option:";
            // 
            // Label15
            // 
            this.Label15.AutoSize = true;
            this.Label15.BackColor = System.Drawing.Color.Transparent;
            this.Label15.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label15.Location = new System.Drawing.Point(43, 128);
            this.Label15.Name = "Label15";
            this.Label15.Size = new System.Drawing.Size(28, 13);
            this.Label15.TabIndex = 7;
            this.Label15.Text = "Left";
            // 
            // drp_crop
            // 
            this.drp_crop.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_crop.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_crop.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_crop.FormattingEnabled = true;
            this.drp_crop.Items.AddRange(new object[] {
            "Automatic",
            "Custom",
            "No Crop"});
            this.drp_crop.Location = new System.Drawing.Point(118, 34);
            this.drp_crop.Name = "drp_crop";
            this.drp_crop.Size = new System.Drawing.Size(123, 21);
            this.drp_crop.TabIndex = 3;
            this.drp_crop.SelectedIndexChanged += new System.EventHandler(this.drp_crop_SelectedIndexChanged);
            // 
            // Check_ChapterMarkers
            // 
            this.Check_ChapterMarkers.AutoSize = true;
            this.Check_ChapterMarkers.BackColor = System.Drawing.Color.Transparent;
            this.Check_ChapterMarkers.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Check_ChapterMarkers.Location = new System.Drawing.Point(16, 32);
            this.Check_ChapterMarkers.Name = "Check_ChapterMarkers";
            this.Check_ChapterMarkers.Size = new System.Drawing.Size(164, 17);
            this.Check_ChapterMarkers.TabIndex = 1;
            this.Check_ChapterMarkers.Text = "Create chapter markers";
            this.Check_ChapterMarkers.UseVisualStyleBackColor = false;
            this.Check_ChapterMarkers.CheckedChanged += new System.EventHandler(this.Check_ChapterMarkers_CheckedChanged);
            // 
            // advancedOptions
            // 
            this.advancedOptions.Controls.Add(this.TabPage1);
            this.advancedOptions.Controls.Add(this.TabPage3);
            this.advancedOptions.Controls.Add(this.TabPage2);
            this.advancedOptions.Controls.Add(this.tab_chapters);
            this.advancedOptions.Controls.Add(this.h264Tab);
            this.advancedOptions.Controls.Add(this.tabPage4);
            this.advancedOptions.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.advancedOptions.Location = new System.Drawing.Point(14, 281);
            this.advancedOptions.Name = "advancedOptions";
            this.advancedOptions.SelectedIndex = 0;
            this.advancedOptions.Size = new System.Drawing.Size(705, 328);
            this.advancedOptions.TabIndex = 5;
            this.advancedOptions.TabStop = false;
            // 
            // tab_chapters
            // 
            this.tab_chapters.BackColor = System.Drawing.Color.Transparent;
            this.tab_chapters.Controls.Add(this.label31);
            this.tab_chapters.Controls.Add(this.data_chpt);
            this.tab_chapters.Controls.Add(this.Check_ChapterMarkers);
            this.tab_chapters.Location = new System.Drawing.Point(4, 22);
            this.tab_chapters.Name = "tab_chapters";
            this.tab_chapters.Size = new System.Drawing.Size(697, 302);
            this.tab_chapters.TabIndex = 6;
            this.tab_chapters.Text = "Chapters";
            // 
            // label31
            // 
            this.label31.AutoSize = true;
            this.label31.BackColor = System.Drawing.Color.Transparent;
            this.label31.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label31.Location = new System.Drawing.Point(13, 13);
            this.label31.Name = "label31";
            this.label31.Size = new System.Drawing.Size(115, 13);
            this.label31.TabIndex = 0;
            this.label31.Text = "Chapter Markers";
            // 
            // h264Tab
            // 
            this.h264Tab.BackColor = System.Drawing.Color.Transparent;
            this.h264Tab.Controls.Add(this.label43);
            this.h264Tab.Controls.Add(this.btn_reset);
            this.h264Tab.Controls.Add(this.rtf_x264Query);
            this.h264Tab.Controls.Add(this.label35);
            this.h264Tab.Controls.Add(this.check_Cabac);
            this.h264Tab.Controls.Add(this.label36);
            this.h264Tab.Controls.Add(this.check_noDCTDecimate);
            this.h264Tab.Controls.Add(this.label37);
            this.h264Tab.Controls.Add(this.check_noFastPSkip);
            this.h264Tab.Controls.Add(this.label39);
            this.h264Tab.Controls.Add(this.drop_trellis);
            this.h264Tab.Controls.Add(this.drop_deblockBeta);
            this.h264Tab.Controls.Add(this.label41);
            this.h264Tab.Controls.Add(this.drop_deblockAlpha);
            this.h264Tab.Controls.Add(this.panel3);
            this.h264Tab.Controls.Add(this.panel1);
            this.h264Tab.Controls.Add(this.panel2);
            this.h264Tab.Controls.Add(this.label44);
            this.h264Tab.Controls.Add(this.check_8x8DCT);
            this.h264Tab.Controls.Add(this.label45);
            this.h264Tab.Controls.Add(this.drop_analysis);
            this.h264Tab.Controls.Add(this.label48);
            this.h264Tab.Controls.Add(this.drop_subpixelMotionEstimation);
            this.h264Tab.Controls.Add(this.label49);
            this.h264Tab.Controls.Add(this.drop_MotionEstimationRange);
            this.h264Tab.Controls.Add(this.label54);
            this.h264Tab.Controls.Add(this.drop_MotionEstimationMethod);
            this.h264Tab.Controls.Add(this.label57);
            this.h264Tab.Controls.Add(this.check_pyrmidalBFrames);
            this.h264Tab.Controls.Add(this.label58);
            this.h264Tab.Controls.Add(this.check_BidirectionalRefinement);
            this.h264Tab.Controls.Add(this.label59);
            this.h264Tab.Controls.Add(this.check_bFrameDistortion);
            this.h264Tab.Controls.Add(this.label60);
            this.h264Tab.Controls.Add(this.check_weightedBFrames);
            this.h264Tab.Controls.Add(this.label61);
            this.h264Tab.Controls.Add(this.drop_directPrediction);
            this.h264Tab.Controls.Add(this.label62);
            this.h264Tab.Controls.Add(this.drop_bFrames);
            this.h264Tab.Controls.Add(this.label63);
            this.h264Tab.Controls.Add(this.label64);
            this.h264Tab.Controls.Add(this.drop_refFrames);
            this.h264Tab.Controls.Add(this.check_mixedReferences);
            this.h264Tab.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.h264Tab.Location = new System.Drawing.Point(4, 22);
            this.h264Tab.Name = "h264Tab";
            this.h264Tab.Padding = new System.Windows.Forms.Padding(3);
            this.h264Tab.Size = new System.Drawing.Size(697, 302);
            this.h264Tab.TabIndex = 8;
            this.h264Tab.Text = "Advanced";
            // 
            // label43
            // 
            this.label43.AutoSize = true;
            this.label43.BackColor = System.Drawing.Color.Transparent;
            this.label43.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label43.Location = new System.Drawing.Point(13, 13);
            this.label43.Name = "label43";
            this.label43.Size = new System.Drawing.Size(165, 13);
            this.label43.TabIndex = 0;
            this.label43.Text = "Advanced H.264 Options";
            // 
            // btn_reset
            // 
            this.btn_reset.Location = new System.Drawing.Point(13, 226);
            this.btn_reset.Name = "btn_reset";
            this.btn_reset.Size = new System.Drawing.Size(75, 23);
            this.btn_reset.TabIndex = 41;
            this.btn_reset.Text = "Reset All";
            this.btn_reset.UseVisualStyleBackColor = true;
            this.btn_reset.Click += new System.EventHandler(this.btn_reset_Click);
            // 
            // rtf_x264Query
            // 
            this.rtf_x264Query.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.rtf_x264Query.Location = new System.Drawing.Point(13, 253);
            this.rtf_x264Query.Name = "rtf_x264Query";
            this.rtf_x264Query.Size = new System.Drawing.Size(667, 43);
            this.rtf_x264Query.TabIndex = 42;
            this.rtf_x264Query.Text = "";
            this.rtf_x264Query.LostFocus += new System.EventHandler(this.rtf_x264Query_TextChanged);
            this.rtf_x264Query.TextChanged += new System.EventHandler(this.rtf_x264Query_TextChanged);
            // 
            // label35
            // 
            this.label35.AutoSize = true;
            this.label35.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label35.Location = new System.Drawing.Point(534, 213);
            this.label35.Name = "label35";
            this.label35.Size = new System.Drawing.Size(128, 12);
            this.label35.TabIndex = 29;
            this.label35.Text = "CABAC Entropy Coding:";
            // 
            // check_Cabac
            // 
            this.check_Cabac.AutoSize = true;
            this.check_Cabac.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_Cabac.Location = new System.Drawing.Point(668, 214);
            this.check_Cabac.Name = "check_Cabac";
            this.check_Cabac.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_Cabac.Size = new System.Drawing.Size(12, 11);
            this.check_Cabac.TabIndex = 40;
            this.check_Cabac.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_Cabac.UseVisualStyleBackColor = true;
            this.check_Cabac.CheckStateChanged += new System.EventHandler(this.check_Cabac_CheckedChanged);
            // 
            // label36
            // 
            this.label36.AutoSize = true;
            this.label36.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label36.Location = new System.Drawing.Point(561, 194);
            this.label36.Name = "label36";
            this.label36.Size = new System.Drawing.Size(102, 12);
            this.label36.TabIndex = 28;
            this.label36.Text = "No DCT-Decimate:";
            // 
            // check_noDCTDecimate
            // 
            this.check_noDCTDecimate.AutoSize = true;
            this.check_noDCTDecimate.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_noDCTDecimate.Location = new System.Drawing.Point(668, 195);
            this.check_noDCTDecimate.Name = "check_noDCTDecimate";
            this.check_noDCTDecimate.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_noDCTDecimate.Size = new System.Drawing.Size(12, 11);
            this.check_noDCTDecimate.TabIndex = 39;
            this.check_noDCTDecimate.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_noDCTDecimate.UseVisualStyleBackColor = true;
            this.check_noDCTDecimate.CheckStateChanged += new System.EventHandler(this.check_noDCTDecimate_CheckedChanged);
            // 
            // label37
            // 
            this.label37.AutoSize = true;
            this.label37.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label37.Location = new System.Drawing.Point(444, 193);
            this.label37.Name = "label37";
            this.label37.Size = new System.Drawing.Size(87, 12);
            this.label37.TabIndex = 27;
            this.label37.Text = "No Fast-P-Skip:";
            // 
            // check_noFastPSkip
            // 
            this.check_noFastPSkip.AutoSize = true;
            this.check_noFastPSkip.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_noFastPSkip.Location = new System.Drawing.Point(537, 194);
            this.check_noFastPSkip.Name = "check_noFastPSkip";
            this.check_noFastPSkip.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_noFastPSkip.Size = new System.Drawing.Size(12, 11);
            this.check_noFastPSkip.TabIndex = 38;
            this.check_noFastPSkip.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_noFastPSkip.UseVisualStyleBackColor = true;
            this.check_noFastPSkip.CheckStateChanged += new System.EventHandler(this.check_noFastPSkip_CheckedChanged);
            // 
            // label39
            // 
            this.label39.AutoSize = true;
            this.label39.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label39.Location = new System.Drawing.Point(490, 169);
            this.label39.Name = "label39";
            this.label39.Size = new System.Drawing.Size(41, 12);
            this.label39.TabIndex = 26;
            this.label39.Text = "Trellis:";
            // 
            // drop_trellis
            // 
            this.drop_trellis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_trellis.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_trellis.FormattingEnabled = true;
            this.drop_trellis.Location = new System.Drawing.Point(537, 166);
            this.drop_trellis.Name = "drop_trellis";
            this.drop_trellis.Size = new System.Drawing.Size(143, 20);
            this.drop_trellis.TabIndex = 37;
            this.drop_trellis.SelectedIndexChanged += new System.EventHandler(this.drop_trellis_SelectedIndexChanged);
            // 
            // drop_deblockBeta
            // 
            this.drop_deblockBeta.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_deblockBeta.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_deblockBeta.FormattingEnabled = true;
            this.drop_deblockBeta.Location = new System.Drawing.Point(611, 139);
            this.drop_deblockBeta.Name = "drop_deblockBeta";
            this.drop_deblockBeta.Size = new System.Drawing.Size(69, 20);
            this.drop_deblockBeta.TabIndex = 36;
            this.drop_deblockBeta.SelectedIndexChanged += new System.EventHandler(this.drop_deblockBeta_SelectedIndexChanged);
            // 
            // label41
            // 
            this.label41.AutoSize = true;
            this.label41.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label41.Location = new System.Drawing.Point(466, 147);
            this.label41.Name = "label41";
            this.label41.Size = new System.Drawing.Size(65, 12);
            this.label41.TabIndex = 25;
            this.label41.Text = "Deblocking:";
            // 
            // drop_deblockAlpha
            // 
            this.drop_deblockAlpha.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_deblockAlpha.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_deblockAlpha.FormattingEnabled = true;
            this.drop_deblockAlpha.Location = new System.Drawing.Point(537, 139);
            this.drop_deblockAlpha.Name = "drop_deblockAlpha";
            this.drop_deblockAlpha.Size = new System.Drawing.Size(68, 20);
            this.drop_deblockAlpha.TabIndex = 35;
            this.drop_deblockAlpha.SelectedIndexChanged += new System.EventHandler(this.drop_deblockAlpha_SelectedIndexChanged);
            // 
            // panel3
            // 
            this.panel3.BackColor = System.Drawing.Color.Black;
            this.panel3.Location = new System.Drawing.Point(396, 131);
            this.panel3.Margin = new System.Windows.Forms.Padding(0);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(284, 1);
            this.panel3.TabIndex = 24;
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.Black;
            this.panel1.Location = new System.Drawing.Point(13, 84);
            this.panel1.Margin = new System.Windows.Forms.Padding(0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(284, 1);
            this.panel1.TabIndex = 3;
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.Color.Black;
            this.panel2.Location = new System.Drawing.Point(396, 95);
            this.panel2.Margin = new System.Windows.Forms.Padding(0);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(284, 1);
            this.panel2.TabIndex = 21;
            // 
            // label44
            // 
            this.label44.AutoSize = true;
            this.label44.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label44.Location = new System.Drawing.Point(590, 108);
            this.label44.Name = "label44";
            this.label44.Size = new System.Drawing.Size(55, 12);
            this.label44.TabIndex = 23;
            this.label44.Text = "8x8 DCT:";
            // 
            // check_8x8DCT
            // 
            this.check_8x8DCT.AutoSize = true;
            this.check_8x8DCT.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_8x8DCT.Location = new System.Drawing.Point(648, 108);
            this.check_8x8DCT.Name = "check_8x8DCT";
            this.check_8x8DCT.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_8x8DCT.Size = new System.Drawing.Size(12, 11);
            this.check_8x8DCT.TabIndex = 34;
            this.check_8x8DCT.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_8x8DCT.UseVisualStyleBackColor = true;
            this.check_8x8DCT.CheckStateChanged += new System.EventHandler(this.check_8x8DCT_CheckedChanged);
            // 
            // label45
            // 
            this.label45.AutoSize = true;
            this.label45.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label45.Location = new System.Drawing.Point(478, 108);
            this.label45.Name = "label45";
            this.label45.Size = new System.Drawing.Size(53, 12);
            this.label45.TabIndex = 22;
            this.label45.Text = "Analysis:";
            // 
            // drop_analysis
            // 
            this.drop_analysis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_analysis.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_analysis.FormattingEnabled = true;
            this.drop_analysis.Location = new System.Drawing.Point(537, 105);
            this.drop_analysis.Name = "drop_analysis";
            this.drop_analysis.Size = new System.Drawing.Size(47, 20);
            this.drop_analysis.TabIndex = 33;
            this.drop_analysis.SelectedIndexChanged += new System.EventHandler(this.drop_analysis_SelectedIndexChanged);
            // 
            // label48
            // 
            this.label48.AutoSize = true;
            this.label48.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label48.Location = new System.Drawing.Point(385, 72);
            this.label48.Name = "label48";
            this.label48.Size = new System.Drawing.Size(146, 12);
            this.label48.TabIndex = 20;
            this.label48.Text = "Subpixel Motion Estimation:";
            // 
            // drop_subpixelMotionEstimation
            // 
            this.drop_subpixelMotionEstimation.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_subpixelMotionEstimation.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_subpixelMotionEstimation.FormattingEnabled = true;
            this.drop_subpixelMotionEstimation.Location = new System.Drawing.Point(537, 69);
            this.drop_subpixelMotionEstimation.Name = "drop_subpixelMotionEstimation";
            this.drop_subpixelMotionEstimation.Size = new System.Drawing.Size(139, 20);
            this.drop_subpixelMotionEstimation.TabIndex = 32;
            this.drop_subpixelMotionEstimation.SelectedIndexChanged += new System.EventHandler(this.drop_subpixelMotionEstimation_SelectedIndexChanged);
            // 
            // label49
            // 
            this.label49.AutoSize = true;
            this.label49.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label49.Location = new System.Drawing.Point(397, 43);
            this.label49.Name = "label49";
            this.label49.Size = new System.Drawing.Size(134, 12);
            this.label49.TabIndex = 19;
            this.label49.Text = "Motion Estimation Range:";
            // 
            // drop_MotionEstimationRange
            // 
            this.drop_MotionEstimationRange.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_MotionEstimationRange.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_MotionEstimationRange.FormattingEnabled = true;
            this.drop_MotionEstimationRange.Location = new System.Drawing.Point(537, 40);
            this.drop_MotionEstimationRange.Name = "drop_MotionEstimationRange";
            this.drop_MotionEstimationRange.Size = new System.Drawing.Size(139, 20);
            this.drop_MotionEstimationRange.TabIndex = 31;
            this.drop_MotionEstimationRange.SelectedIndexChanged += new System.EventHandler(this.drop_MotionEstimationRange_SelectedIndexChanged);
            // 
            // label54
            // 
            this.label54.AutoSize = true;
            this.label54.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label54.Location = new System.Drawing.Point(391, 14);
            this.label54.Name = "label54";
            this.label54.Size = new System.Drawing.Size(140, 12);
            this.label54.TabIndex = 18;
            this.label54.Text = "Motion Estimation Method:";
            // 
            // drop_MotionEstimationMethod
            // 
            this.drop_MotionEstimationMethod.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_MotionEstimationMethod.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_MotionEstimationMethod.FormattingEnabled = true;
            this.drop_MotionEstimationMethod.ItemHeight = 12;
            this.drop_MotionEstimationMethod.Location = new System.Drawing.Point(537, 11);
            this.drop_MotionEstimationMethod.Name = "drop_MotionEstimationMethod";
            this.drop_MotionEstimationMethod.Size = new System.Drawing.Size(139, 20);
            this.drop_MotionEstimationMethod.TabIndex = 30;
            this.drop_MotionEstimationMethod.SelectedIndexChanged += new System.EventHandler(this.drop_MotionEstimationMethod_SelectedIndexChanged);
            // 
            // label57
            // 
            this.label57.AutoSize = true;
            this.label57.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label57.Location = new System.Drawing.Point(41, 204);
            this.label57.Name = "label57";
            this.label57.Size = new System.Drawing.Size(105, 12);
            this.label57.TabIndex = 9;
            this.label57.Text = "Pyrmidal B-Frames:";
            // 
            // check_pyrmidalBFrames
            // 
            this.check_pyrmidalBFrames.AutoSize = true;
            this.check_pyrmidalBFrames.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_pyrmidalBFrames.Location = new System.Drawing.Point(157, 205);
            this.check_pyrmidalBFrames.Name = "check_pyrmidalBFrames";
            this.check_pyrmidalBFrames.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_pyrmidalBFrames.Size = new System.Drawing.Size(12, 11);
            this.check_pyrmidalBFrames.TabIndex = 17;
            this.check_pyrmidalBFrames.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_pyrmidalBFrames.UseVisualStyleBackColor = true;
            this.check_pyrmidalBFrames.CheckStateChanged += new System.EventHandler(this.check_pyrmidalBFrames_CheckedChanged);
            // 
            // label58
            // 
            this.label58.AutoSize = true;
            this.label58.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label58.Location = new System.Drawing.Point(15, 184);
            this.label58.Name = "label58";
            this.label58.Size = new System.Drawing.Size(131, 12);
            this.label58.TabIndex = 8;
            this.label58.Text = "Bidirectional Refinement:";
            // 
            // check_BidirectionalRefinement
            // 
            this.check_BidirectionalRefinement.AutoSize = true;
            this.check_BidirectionalRefinement.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_BidirectionalRefinement.Location = new System.Drawing.Point(157, 185);
            this.check_BidirectionalRefinement.Name = "check_BidirectionalRefinement";
            this.check_BidirectionalRefinement.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_BidirectionalRefinement.Size = new System.Drawing.Size(12, 11);
            this.check_BidirectionalRefinement.TabIndex = 16;
            this.check_BidirectionalRefinement.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_BidirectionalRefinement.UseVisualStyleBackColor = true;
            this.check_BidirectionalRefinement.CheckStateChanged += new System.EventHandler(this.check_BidirectionalRefinement_CheckedChanged);
            // 
            // label59
            // 
            this.label59.AutoSize = true;
            this.label59.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label59.Location = new System.Drawing.Point(15, 164);
            this.label59.Name = "label59";
            this.label59.Size = new System.Drawing.Size(131, 12);
            this.label59.TabIndex = 7;
            this.label59.Text = "B-Frame Rate Distortion:";
            // 
            // check_bFrameDistortion
            // 
            this.check_bFrameDistortion.AutoSize = true;
            this.check_bFrameDistortion.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_bFrameDistortion.Location = new System.Drawing.Point(157, 165);
            this.check_bFrameDistortion.Name = "check_bFrameDistortion";
            this.check_bFrameDistortion.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_bFrameDistortion.Size = new System.Drawing.Size(12, 11);
            this.check_bFrameDistortion.TabIndex = 15;
            this.check_bFrameDistortion.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_bFrameDistortion.UseVisualStyleBackColor = true;
            this.check_bFrameDistortion.CheckStateChanged += new System.EventHandler(this.check_bFrameDistortion_CheckedChanged);
            // 
            // label60
            // 
            this.label60.AutoSize = true;
            this.label60.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label60.Location = new System.Drawing.Point(39, 146);
            this.label60.Name = "label60";
            this.label60.Size = new System.Drawing.Size(107, 12);
            this.label60.TabIndex = 6;
            this.label60.Text = "Weighted B-Frames:";
            // 
            // check_weightedBFrames
            // 
            this.check_weightedBFrames.AutoSize = true;
            this.check_weightedBFrames.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_weightedBFrames.Location = new System.Drawing.Point(157, 147);
            this.check_weightedBFrames.Name = "check_weightedBFrames";
            this.check_weightedBFrames.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_weightedBFrames.Size = new System.Drawing.Size(12, 11);
            this.check_weightedBFrames.TabIndex = 14;
            this.check_weightedBFrames.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_weightedBFrames.UseVisualStyleBackColor = true;
            this.check_weightedBFrames.CheckStateChanged += new System.EventHandler(this.check_weightedBFrames_CheckedChanged);
            // 
            // label61
            // 
            this.label61.AutoSize = true;
            this.label61.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label61.Location = new System.Drawing.Point(52, 121);
            this.label61.Name = "label61";
            this.label61.Size = new System.Drawing.Size(94, 12);
            this.label61.TabIndex = 5;
            this.label61.Text = "Direct Prediction:";
            // 
            // drop_directPrediction
            // 
            this.drop_directPrediction.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_directPrediction.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_directPrediction.FormattingEnabled = true;
            this.drop_directPrediction.Location = new System.Drawing.Point(157, 118);
            this.drop_directPrediction.Name = "drop_directPrediction";
            this.drop_directPrediction.Size = new System.Drawing.Size(121, 20);
            this.drop_directPrediction.TabIndex = 13;
            this.drop_directPrediction.SelectedIndexChanged += new System.EventHandler(this.drop_directPrediction_SelectedIndexChanged);
            // 
            // label62
            // 
            this.label62.AutoSize = true;
            this.label62.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label62.Location = new System.Drawing.Point(88, 94);
            this.label62.Name = "label62";
            this.label62.Size = new System.Drawing.Size(58, 12);
            this.label62.TabIndex = 4;
            this.label62.Text = "B-Frames:";
            // 
            // drop_bFrames
            // 
            this.drop_bFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_bFrames.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_bFrames.FormattingEnabled = true;
            this.drop_bFrames.Location = new System.Drawing.Point(157, 91);
            this.drop_bFrames.Name = "drop_bFrames";
            this.drop_bFrames.Size = new System.Drawing.Size(121, 20);
            this.drop_bFrames.TabIndex = 12;
            this.drop_bFrames.SelectedIndexChanged += new System.EventHandler(this.drop_bFrames_SelectedIndexChanged);
            // 
            // label63
            // 
            this.label63.AutoSize = true;
            this.label63.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label63.Location = new System.Drawing.Point(48, 64);
            this.label63.Name = "label63";
            this.label63.Size = new System.Drawing.Size(98, 12);
            this.label63.TabIndex = 2;
            this.label63.Text = "Mixed References:";
            // 
            // label64
            // 
            this.label64.AutoSize = true;
            this.label64.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label64.Location = new System.Drawing.Point(47, 40);
            this.label64.Name = "label64";
            this.label64.Size = new System.Drawing.Size(99, 12);
            this.label64.TabIndex = 1;
            this.label64.Text = "Reference Frames:";
            // 
            // drop_refFrames
            // 
            this.drop_refFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_refFrames.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_refFrames.FormattingEnabled = true;
            this.drop_refFrames.Location = new System.Drawing.Point(157, 37);
            this.drop_refFrames.Name = "drop_refFrames";
            this.drop_refFrames.Size = new System.Drawing.Size(121, 20);
            this.drop_refFrames.TabIndex = 10;
            this.drop_refFrames.SelectedIndexChanged += new System.EventHandler(this.drop_refFrames_SelectedIndexChanged);
            // 
            // check_mixedReferences
            // 
            this.check_mixedReferences.AutoSize = true;
            this.check_mixedReferences.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_mixedReferences.Location = new System.Drawing.Point(157, 65);
            this.check_mixedReferences.Name = "check_mixedReferences";
            this.check_mixedReferences.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_mixedReferences.Size = new System.Drawing.Size(12, 11);
            this.check_mixedReferences.TabIndex = 11;
            this.check_mixedReferences.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_mixedReferences.UseVisualStyleBackColor = true;
            this.check_mixedReferences.CheckStateChanged += new System.EventHandler(this.check_mixedReferences_CheckedChanged);
            // 
            // tabPage4
            // 
            this.tabPage4.Controls.Add(this.btn_clear);
            this.tabPage4.Controls.Add(this.btn_copy2C);
            this.tabPage4.Controls.Add(this.label34);
            this.tabPage4.Controls.Add(this.btn_generate_Query);
            this.tabPage4.Controls.Add(this.label33);
            this.tabPage4.Controls.Add(this.rtf_query);
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Size = new System.Drawing.Size(697, 302);
            this.tabPage4.TabIndex = 7;
            this.tabPage4.Text = "Query Editor";
            // 
            // btn_clear
            // 
            this.btn_clear.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_clear.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_clear.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_clear.Location = new System.Drawing.Point(602, 75);
            this.btn_clear.Name = "btn_clear";
            this.btn_clear.Size = new System.Drawing.Size(75, 22);
            this.btn_clear.TabIndex = 4;
            this.btn_clear.Text = "Clear";
            this.btn_clear.UseVisualStyleBackColor = true;
            this.btn_clear.Click += new System.EventHandler(this.btn_clear_Click);
            // 
            // btn_copy2C
            // 
            this.btn_copy2C.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_copy2C.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_copy2C.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_copy2C.Location = new System.Drawing.Point(148, 75);
            this.btn_copy2C.Name = "btn_copy2C";
            this.btn_copy2C.Size = new System.Drawing.Size(136, 22);
            this.btn_copy2C.TabIndex = 3;
            this.btn_copy2C.Text = "Copy to clipboard";
            this.btn_copy2C.UseVisualStyleBackColor = true;
            this.btn_copy2C.Click += new System.EventHandler(this.btn_copy2C_Click);
            // 
            // label34
            // 
            this.label34.AutoSize = true;
            this.label34.Location = new System.Drawing.Point(13, 36);
            this.label34.Name = "label34";
            this.label34.Size = new System.Drawing.Size(434, 26);
            this.label34.TabIndex = 1;
            this.label34.Text = "Here you can alter the query generated by this program.\r\nDon\'t forget to re-gener" +
                "ate this query should you change any GUI options.";
            // 
            // btn_generate_Query
            // 
            this.btn_generate_Query.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_generate_Query.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_generate_Query.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_generate_Query.Location = new System.Drawing.Point(16, 75);
            this.btn_generate_Query.Name = "btn_generate_Query";
            this.btn_generate_Query.Size = new System.Drawing.Size(126, 22);
            this.btn_generate_Query.TabIndex = 2;
            this.btn_generate_Query.Text = "Generate Query";
            this.btn_generate_Query.UseVisualStyleBackColor = true;
            this.btn_generate_Query.Click += new System.EventHandler(this.btn_generate_Query_Click);
            // 
            // label33
            // 
            this.label33.AutoSize = true;
            this.label33.BackColor = System.Drawing.Color.Transparent;
            this.label33.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label33.Location = new System.Drawing.Point(13, 13);
            this.label33.Name = "label33";
            this.label33.Size = new System.Drawing.Size(89, 13);
            this.label33.TabIndex = 0;
            this.label33.Text = "Query Editor";
            // 
            // rtf_query
            // 
            this.rtf_query.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.rtf_query.Location = new System.Drawing.Point(16, 103);
            this.rtf_query.Name = "rtf_query";
            this.rtf_query.Size = new System.Drawing.Size(661, 182);
            this.rtf_query.TabIndex = 5;
            this.rtf_query.Text = "";
            // 
            // groupBox_dest
            // 
            this.groupBox_dest.Controls.Add(this.btn_destBrowse);
            this.groupBox_dest.Controls.Add(this.Label3);
            this.groupBox_dest.Controls.Add(this.text_destination);
            this.groupBox_dest.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox_dest.ForeColor = System.Drawing.Color.Black;
            this.groupBox_dest.Location = new System.Drawing.Point(14, 163);
            this.groupBox_dest.Name = "groupBox_dest";
            this.groupBox_dest.Size = new System.Drawing.Size(705, 50);
            this.groupBox_dest.TabIndex = 2;
            this.groupBox_dest.TabStop = false;
            this.groupBox_dest.Text = "Destination";
            // 
            // btn_destBrowse
            // 
            this.btn_destBrowse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_destBrowse.Location = new System.Drawing.Point(598, 18);
            this.btn_destBrowse.Name = "btn_destBrowse";
            this.btn_destBrowse.Size = new System.Drawing.Size(75, 23);
            this.btn_destBrowse.TabIndex = 13;
            this.btn_destBrowse.Text = "Browse";
            this.btn_destBrowse.UseVisualStyleBackColor = true;
            this.btn_destBrowse.Click += new System.EventHandler(this.btn_destBrowse_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.btn_removePreset);
            this.groupBox2.Controls.Add(this.btn_addPreset);
            this.groupBox2.Controls.Add(this.treeView_presets);
            this.groupBox2.Controls.Add(this.btn_setDefault);
            this.groupBox2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox2.ForeColor = System.Drawing.Color.Black;
            this.groupBox2.Location = new System.Drawing.Point(729, 73);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(198, 536);
            this.groupBox2.TabIndex = 11;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Presets";
            // 
            // treeView_presets
            // 
            this.treeView_presets.ForeColor = System.Drawing.Color.Navy;
            this.treeView_presets.FullRowSelect = true;
            this.treeView_presets.HideSelection = false;
            this.treeView_presets.ItemHeight = 17;
            this.treeView_presets.Location = new System.Drawing.Point(10, 23);
            this.treeView_presets.Name = "treeView_presets";
            this.treeView_presets.ShowLines = false;
            this.treeView_presets.Size = new System.Drawing.Size(177, 473);
            this.treeView_presets.TabIndex = 2;
            this.treeView_presets.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView_presets_AfterSelect);
            // 
            // toolStrip1
            // 
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_start,
            this.toolStripSeparator1,
            this.btn_add2Queue,
            this.btn_showQueue,
            this.toolStripSeparator4,
            this.btn_ActivityWindow,
            this.toolStripSeparator8});
            this.toolStrip1.Location = new System.Drawing.Point(0, 24);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(931, 39);
            this.toolStrip1.TabIndex = 13;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // btn_start
            // 
            this.btn_start.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_start.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_start.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_start.Name = "btn_start";
            this.btn_start.Size = new System.Drawing.Size(67, 36);
            this.btn_start.Text = "Start";
            this.btn_start.ToolTipText = "Start the encoding process";
            this.btn_start.Click += new System.EventHandler(this.btn_start_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 39);
            // 
            // btn_add2Queue
            // 
            this.btn_add2Queue.Image = global::Handbrake.Properties.Resources.AddToQueue;
            this.btn_add2Queue.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_add2Queue.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_add2Queue.Name = "btn_add2Queue";
            this.btn_add2Queue.Size = new System.Drawing.Size(110, 36);
            this.btn_add2Queue.Text = "Add to Queue";
            this.btn_add2Queue.ToolTipText = "Add a new item to the Queue";
            this.btn_add2Queue.Click += new System.EventHandler(this.btn_add2Queue_Click);
            // 
            // btn_showQueue
            // 
            this.btn_showQueue.Image = global::Handbrake.Properties.Resources.Queue;
            this.btn_showQueue.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_showQueue.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_showQueue.Name = "btn_showQueue";
            this.btn_showQueue.Size = new System.Drawing.Size(104, 36);
            this.btn_showQueue.Tag = "";
            this.btn_showQueue.Text = "Show Queue";
            this.btn_showQueue.Click += new System.EventHandler(this.btn_showQueue_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(6, 39);
            // 
            // btn_ActivityWindow
            // 
            this.btn_ActivityWindow.Image = global::Handbrake.Properties.Resources.ActivityWindow;
            this.btn_ActivityWindow.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_ActivityWindow.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_ActivityWindow.Name = "btn_ActivityWindow";
            this.btn_ActivityWindow.Size = new System.Drawing.Size(120, 36);
            this.btn_ActivityWindow.Text = "Activity Window";
            this.btn_ActivityWindow.ToolTipText = "Displays the activity window which displays the log of the last completed or curr" +
                "ently running encode.";
            this.btn_ActivityWindow.Click += new System.EventHandler(this.btn_ActivityWindow_Click);
            // 
            // toolStripSeparator8
            // 
            this.toolStripSeparator8.Name = "toolStripSeparator8";
            this.toolStripSeparator8.Size = new System.Drawing.Size(6, 39);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(620, 29);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 3;
            this.button1.Text = "load";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(699, 29);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(75, 23);
            this.button2.TabIndex = 15;
            this.button2.Text = "save";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(931, 617);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.lbl_encode);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox_dest);
            this.Controls.Add(this.advancedOptions);
            this.Controls.Add(this.groupBox_output);
            this.Controls.Add(this.GroupBox1);
            this.Controls.Add(this.frmMainMenu);
            this.DoubleBuffered = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(680, 580);
            this.Name = "frmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Handbrake";
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.data_chpt)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.slider_drc)).EndInit();
            this.frmMainMenu.ResumeLayout(false);
            this.frmMainMenu.PerformLayout();
            this.GroupBox1.ResumeLayout(false);
            this.GroupBox1.PerformLayout();
            this.groupBox_output.ResumeLayout(false);
            this.groupBox_output.PerformLayout();
            this.TabPage2.ResumeLayout(false);
            this.TabPage2.PerformLayout();
            this.TabPage3.ResumeLayout(false);
            this.TabPage3.PerformLayout();
            this.TabPage1.ResumeLayout(false);
            this.TabPage1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.text_bottom)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_top)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_left)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_right)).EndInit();
            this.advancedOptions.ResumeLayout(false);
            this.tab_chapters.ResumeLayout(false);
            this.tab_chapters.PerformLayout();
            this.h264Tab.ResumeLayout(false);
            this.h264Tab.PerformLayout();
            this.tabPage4.ResumeLayout(false);
            this.tabPage4.PerformLayout();
            this.groupBox_dest.ResumeLayout(false);
            this.groupBox_dest.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.SaveFileDialog File_Save;
        internal System.Windows.Forms.ToolTip ToolTip;
        internal System.Windows.Forms.ToolStripMenuItem FileToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_open3;
        internal System.Windows.Forms.ToolStripMenuItem mnu_exit;
        internal System.Windows.Forms.ToolStripMenuItem ToolsToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_encode;
        internal System.Windows.Forms.ToolStripMenuItem mnu_viewDVDdata;
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator5;
        internal System.Windows.Forms.ToolStripMenuItem mnu_options;
        internal System.Windows.Forms.ToolStripMenuItem PresetsToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_presetReset;
        internal System.Windows.Forms.ToolStripMenuItem HelpToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem OnlineDocumentationToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_wiki;
        internal System.Windows.Forms.ToolStripMenuItem mnu_onlineDocs;
        internal System.Windows.Forms.ToolStripMenuItem mnu_faq;
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator3;
        internal System.Windows.Forms.ToolStripMenuItem mnu_about;
        internal System.Windows.Forms.MenuStrip frmMainMenu;
        internal System.Windows.Forms.GroupBox GroupBox1;
        internal System.Windows.Forms.Label Label13;
        internal System.Windows.Forms.ComboBox drop_chapterFinish;
        internal System.Windows.Forms.ComboBox drop_chapterStart;
        internal System.Windows.Forms.ComboBox drp_dvdtitle;
        internal System.Windows.Forms.Label Label17;
        internal System.Windows.Forms.TextBox text_source;
        internal System.Windows.Forms.Label Label9;
        internal System.Windows.Forms.Label Label10;
        internal System.Windows.Forms.GroupBox groupBox_output;
        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.ComboBox drp_videoEncoder;
        internal System.Windows.Forms.Label Label47;
        internal System.Windows.Forms.TextBox text_destination;
        private System.Windows.Forms.Label lbl_encode;
        internal System.Windows.Forms.TabPage TabPage2;
        internal System.Windows.Forms.ComboBox drp_audmix_1;
        internal System.Windows.Forms.ComboBox drp_track1Audio;
        internal System.Windows.Forms.ComboBox drp_audbit_1;
        internal System.Windows.Forms.Label Label5;
        internal System.Windows.Forms.Label Label32;
        internal System.Windows.Forms.ComboBox drp_audsr_1;
        internal System.Windows.Forms.TabPage TabPage3;
        internal System.Windows.Forms.CheckBox check_largeFile;
        internal System.Windows.Forms.CheckBox check_turbo;
        internal System.Windows.Forms.Label Label22;
        internal System.Windows.Forms.Label Label2;
        internal System.Windows.Forms.Label SliderValue;
        internal System.Windows.Forms.ComboBox drp_videoFramerate;
        internal System.Windows.Forms.CheckBox check_2PassEncode;
        internal System.Windows.Forms.TrackBar slider_videoQuality;
        internal System.Windows.Forms.TextBox text_filesize;
        internal System.Windows.Forms.Label Label40;
        internal System.Windows.Forms.TextBox text_bitrate;
        internal System.Windows.Forms.Label Label42;
        internal System.Windows.Forms.TabPage TabPage1;
        internal System.Windows.Forms.CheckBox Check_ChapterMarkers;
        internal System.Windows.Forms.Label lbl_RecomendedCrop;
        internal System.Windows.Forms.Label Label8;
        internal System.Windows.Forms.Label Label1;
        internal System.Windows.Forms.Label Label53;
        internal System.Windows.Forms.Label Label52;
        internal System.Windows.Forms.Label Label51;
        internal System.Windows.Forms.Label Label50;
        internal System.Windows.Forms.Label Label15;
        internal System.Windows.Forms.ComboBox drp_crop;
        internal System.Windows.Forms.TabControl advancedOptions;
        internal System.Windows.Forms.Label Label46;
        private System.Windows.Forms.GroupBox groupBox_dest;
        internal System.Windows.Forms.ComboBox drp_subtitle;
        internal System.Windows.Forms.Label Label19;
        internal System.Windows.Forms.Label Label20;
        internal System.Windows.Forms.CheckBox check_grayscale;
        internal System.Windows.Forms.Label label24;
        internal System.Windows.Forms.ComboBox drp_deNoise;
        internal System.Windows.Forms.Label label11;
        internal System.Windows.Forms.CheckBox check_deblock;
        internal System.Windows.Forms.CheckBox check_detelecine;
        internal System.Windows.Forms.Label label4;
        internal System.Windows.Forms.ComboBox drp_deInterlace_option;
        private System.Windows.Forms.GroupBox groupBox2;
        internal System.Windows.Forms.Button btn_setDefault;
        private System.Windows.Forms.ToolStripMenuItem mnu_SelectDefault;
        private System.Windows.Forms.ToolStripMenuItem mnu_UpdateCheck;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator6;
        private System.Windows.Forms.SaveFileDialog DVD_Save;
        private System.Windows.Forms.OpenFileDialog File_Open;
        private System.Windows.Forms.OpenFileDialog ISO_Open;
        private System.Windows.Forms.FolderBrowserDialog DVD_Open;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem mnu_open;
        private System.Windows.Forms.ToolStripMenuItem mnu_save;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator7;
        private System.Windows.Forms.TreeView treeView_presets;
        internal System.Windows.Forms.CheckBox check_vfr;
        internal System.Windows.Forms.CheckBox check_iPodAtom;
        internal System.Windows.Forms.Label lbl_vfr;
        internal System.Windows.Forms.Label label26;
        internal System.Windows.Forms.Label Label56;
        internal System.Windows.Forms.Label lbl_Aspect;
        internal System.Windows.Forms.Label Label91;
        internal System.Windows.Forms.TextBox text_height;
        internal System.Windows.Forms.Label Label55;
        internal System.Windows.Forms.TextBox text_width;
        internal System.Windows.Forms.Label label27;
        internal System.Windows.Forms.ComboBox drp_track2Audio;
        internal System.Windows.Forms.Label label28;
        internal System.Windows.Forms.TrackBar slider_drc;
        internal System.Windows.Forms.Label lbl_drc;
        private System.Windows.Forms.Label label30;
        private System.Windows.Forms.Label label29;
        private System.Windows.Forms.Label label23;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TabPage tab_chapters;
        internal System.Windows.Forms.Label label31;
        internal System.Windows.Forms.NumericUpDown text_right;
        internal System.Windows.Forms.NumericUpDown text_bottom;
        internal System.Windows.Forms.NumericUpDown text_top;
        internal System.Windows.Forms.NumericUpDown text_left;
        internal System.Windows.Forms.CheckBox check_optimiseMP4;
        internal System.Windows.Forms.CheckBox check_forced;
        internal System.Windows.Forms.DataGridView data_chpt;
        private System.Windows.Forms.TabPage tabPage4;
        private System.Windows.Forms.Label label34;
        internal System.Windows.Forms.Button btn_generate_Query;
        internal System.Windows.Forms.Label label33;
        private System.Windows.Forms.RichTextBox rtf_query;
        internal System.Windows.Forms.Button btn_clear;
        internal System.Windows.Forms.Button btn_copy2C;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton btn_start;
        private System.Windows.Forms.ToolStripButton btn_add2Queue;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton btn_showQueue;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.ToolStripButton btn_ActivityWindow;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator8;
        private System.Windows.Forms.ToolStripMenuItem mnu_handbrake_home;
        internal System.Windows.Forms.Button btn_removePreset;
        internal System.Windows.Forms.Button btn_addPreset;
        internal System.Windows.Forms.Label label25;
        internal System.Windows.Forms.Label label6;
        internal System.Windows.Forms.ComboBox drp_anamorphic;
        internal System.Windows.Forms.TabPage h264Tab;
        internal System.Windows.Forms.RichTextBox rtf_x264Query;
        internal System.Windows.Forms.Label label43;
        internal System.Windows.Forms.Button btn_reset;
        internal System.Windows.Forms.Label label35;
        internal System.Windows.Forms.CheckBox check_Cabac;
        internal System.Windows.Forms.Label label36;
        internal System.Windows.Forms.CheckBox check_noDCTDecimate;
        internal System.Windows.Forms.Label label37;
        internal System.Windows.Forms.CheckBox check_noFastPSkip;
        internal System.Windows.Forms.Label label39;
        internal System.Windows.Forms.ComboBox drop_trellis;
        internal System.Windows.Forms.ComboBox drop_deblockBeta;
        internal System.Windows.Forms.Label label41;
        internal System.Windows.Forms.ComboBox drop_deblockAlpha;
        internal System.Windows.Forms.Panel panel3;
        internal System.Windows.Forms.Panel panel1;
        internal System.Windows.Forms.Panel panel2;
        internal System.Windows.Forms.Label label44;
        internal System.Windows.Forms.CheckBox check_8x8DCT;
        internal System.Windows.Forms.Label label45;
        internal System.Windows.Forms.ComboBox drop_analysis;
        internal System.Windows.Forms.Label label48;
        internal System.Windows.Forms.ComboBox drop_subpixelMotionEstimation;
        internal System.Windows.Forms.Label label49;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationRange;
        internal System.Windows.Forms.Label label54;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationMethod;
        internal System.Windows.Forms.Label label57;
        internal System.Windows.Forms.CheckBox check_pyrmidalBFrames;
        internal System.Windows.Forms.Label label58;
        internal System.Windows.Forms.CheckBox check_BidirectionalRefinement;
        internal System.Windows.Forms.Label label59;
        internal System.Windows.Forms.CheckBox check_bFrameDistortion;
        internal System.Windows.Forms.Label label60;
        internal System.Windows.Forms.CheckBox check_weightedBFrames;
        internal System.Windows.Forms.Label label61;
        internal System.Windows.Forms.ComboBox drop_directPrediction;
        internal System.Windows.Forms.Label label62;
        internal System.Windows.Forms.ComboBox drop_bFrames;
        internal System.Windows.Forms.Label label63;
        internal System.Windows.Forms.Label label64;
        internal System.Windows.Forms.ComboBox drop_refFrames;
        internal System.Windows.Forms.CheckBox check_mixedReferences;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button button2;
        internal System.Windows.Forms.ComboBox drp_audmix_2;
        internal System.Windows.Forms.Label label65;
        internal System.Windows.Forms.Label label14;
        internal System.Windows.Forms.ComboBox drp_audenc_2;
        internal System.Windows.Forms.ComboBox drp_audenc_1;
        internal System.Windows.Forms.Label label66;
        internal System.Windows.Forms.ComboBox drp_audbit_2;
        internal System.Windows.Forms.Label label67;
        internal System.Windows.Forms.ComboBox drp_audsr_2;
        internal System.Windows.Forms.Label label68;
        private System.Windows.Forms.DataGridViewTextBoxColumn number;
        private System.Windows.Forms.DataGridViewTextBoxColumn name;
        internal System.Windows.Forms.Label label12;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.CheckBox check_fileMode;
        private System.Windows.Forms.Button btn_Browse;
        private System.Windows.Forms.Button btn_destBrowse;

    }
}