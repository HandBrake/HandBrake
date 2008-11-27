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
            System.Windows.Forms.ContextMenuStrip notifyIconMenu;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmMain));
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            this.btn_restore = new System.Windows.Forms.ToolStripMenuItem();
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
            this.drp_audsr_3 = new System.Windows.Forms.ComboBox();
            this.drp_audbit_3 = new System.Windows.Forms.ComboBox();
            this.drp_audenc_3 = new System.Windows.Forms.ComboBox();
            this.drp_audmix_3 = new System.Windows.Forms.ComboBox();
            this.drp_audsr_4 = new System.Windows.Forms.ComboBox();
            this.drp_audbit_4 = new System.Windows.Forms.ComboBox();
            this.drp_audenc_4 = new System.Windows.Forms.ComboBox();
            this.drp_audmix_4 = new System.Windows.Forms.ComboBox();
            this.drop_format = new System.Windows.Forms.ComboBox();
            this.check_customCrop = new System.Windows.Forms.RadioButton();
            this.check_autoCrop = new System.Windows.Forms.RadioButton();
            this.check_Cabac = new System.Windows.Forms.CheckBox();
            this.check_noDCTDecimate = new System.Windows.Forms.CheckBox();
            this.check_noFastPSkip = new System.Windows.Forms.CheckBox();
            this.drop_trellis = new System.Windows.Forms.ComboBox();
            this.drop_deblockBeta = new System.Windows.Forms.ComboBox();
            this.drop_deblockAlpha = new System.Windows.Forms.ComboBox();
            this.check_8x8DCT = new System.Windows.Forms.CheckBox();
            this.drop_analysis = new System.Windows.Forms.ComboBox();
            this.drop_subpixelMotionEstimation = new System.Windows.Forms.ComboBox();
            this.drop_MotionEstimationRange = new System.Windows.Forms.ComboBox();
            this.drop_MotionEstimationMethod = new System.Windows.Forms.ComboBox();
            this.check_pyrmidalBFrames = new System.Windows.Forms.CheckBox();
            this.check_weightedBFrames = new System.Windows.Forms.CheckBox();
            this.drop_directPrediction = new System.Windows.Forms.ComboBox();
            this.drop_bFrames = new System.Windows.Forms.ComboBox();
            this.drop_refFrames = new System.Windows.Forms.ComboBox();
            this.check_mixedReferences = new System.Windows.Forms.CheckBox();
            this.lbl_src_res = new System.Windows.Forms.Label();
            this.lbl_duration = new System.Windows.Forms.Label();
            this.label_duration = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.DVD_Open = new System.Windows.Forms.FolderBrowserDialog();
            this.File_Open = new System.Windows.Forms.OpenFileDialog();
            this.ISO_Open = new System.Windows.Forms.OpenFileDialog();
            this.FileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_exit = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_open3 = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_encode = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_encodeLog = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_options = new System.Windows.Forms.ToolStripMenuItem();
            this.PresetsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_presetReset = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_delete_preset = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator7 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_new_preset = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_SelectDefault = new System.Windows.Forms.ToolStripMenuItem();
            this.HelpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_user_guide = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_handbrake_home = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_handbrake_forums = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_UpdateCheck = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_about = new System.Windows.Forms.ToolStripMenuItem();
            this.frmMainMenu = new System.Windows.Forms.MenuStrip();
            this.GroupBox1 = new System.Windows.Forms.GroupBox();
            this.Label13 = new System.Windows.Forms.Label();
            this.Label17 = new System.Windows.Forms.Label();
            this.Label9 = new System.Windows.Forms.Label();
            this.Label10 = new System.Windows.Forms.Label();
            this.groupBox_output = new System.Windows.Forms.GroupBox();
            this.label5 = new System.Windows.Forms.Label();
            this.Label47 = new System.Windows.Forms.Label();
            this.Label3 = new System.Windows.Forms.Label();
            this.TabPage2 = new System.Windows.Forms.TabPage();
            this.lbl_drc4 = new System.Windows.Forms.Label();
            this.lbl_drc3 = new System.Windows.Forms.Label();
            this.lbl_drc2 = new System.Windows.Forms.Label();
            this.lbl_drc1 = new System.Windows.Forms.Label();
            this.trackBar4 = new System.Windows.Forms.TrackBar();
            this.drp_track4Audio = new System.Windows.Forms.ComboBox();
            this.lbl_t4 = new System.Windows.Forms.Label();
            this.trackBar3 = new System.Windows.Forms.TrackBar();
            this.drp_track3Audio = new System.Windows.Forms.ComboBox();
            this.lbl_t3 = new System.Windows.Forms.Label();
            this.trackBar2 = new System.Windows.Forms.TrackBar();
            this.label16 = new System.Windows.Forms.Label();
            this.trackBar1 = new System.Windows.Forms.TrackBar();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.label68 = new System.Windows.Forms.Label();
            this.label67 = new System.Windows.Forms.Label();
            this.label66 = new System.Windows.Forms.Label();
            this.label65 = new System.Windows.Forms.Label();
            this.label14 = new System.Windows.Forms.Label();
            this.check_forced = new System.Windows.Forms.CheckBox();
            this.drp_track2Audio = new System.Windows.Forms.ComboBox();
            this.label28 = new System.Windows.Forms.Label();
            this.label27 = new System.Windows.Forms.Label();
            this.Label19 = new System.Windows.Forms.Label();
            this.Label20 = new System.Windows.Forms.Label();
            this.drp_track1Audio = new System.Windows.Forms.ComboBox();
            this.Label32 = new System.Windows.Forms.Label();
            this.TabPage3 = new System.Windows.Forms.TabPage();
            this.label25 = new System.Windows.Forms.Label();
            this.check_grayscale = new System.Windows.Forms.CheckBox();
            this.Label22 = new System.Windows.Forms.Label();
            this.check_2PassEncode = new System.Windows.Forms.CheckBox();
            this.Label2 = new System.Windows.Forms.Label();
            this.Label42 = new System.Windows.Forms.Label();
            this.SliderValue = new System.Windows.Forms.Label();
            this.Label46 = new System.Windows.Forms.Label();
            this.Label40 = new System.Windows.Forms.Label();
            this.TabPage1 = new System.Windows.Forms.TabPage();
            this.slider_deblock = new System.Windows.Forms.TrackBar();
            this.label8 = new System.Windows.Forms.Label();
            this.lbl_deblockVal = new System.Windows.Forms.Label();
            this.check_decomb = new System.Windows.Forms.CheckBox();
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
            this.label24 = new System.Windows.Forms.Label();
            this.drp_deNoise = new System.Windows.Forms.ComboBox();
            this.label11 = new System.Windows.Forms.Label();
            this.check_detelecine = new System.Windows.Forms.CheckBox();
            this.label4 = new System.Windows.Forms.Label();
            this.drp_deInterlace_option = new System.Windows.Forms.ComboBox();
            this.Label1 = new System.Windows.Forms.Label();
            this.Label53 = new System.Windows.Forms.Label();
            this.Label52 = new System.Windows.Forms.Label();
            this.Label51 = new System.Windows.Forms.Label();
            this.Label15 = new System.Windows.Forms.Label();
            this.Check_ChapterMarkers = new System.Windows.Forms.CheckBox();
            this.advancedOptions = new System.Windows.Forms.TabControl();
            this.tab_chapters = new System.Windows.Forms.TabPage();
            this.label31 = new System.Windows.Forms.Label();
            this.h264Tab = new System.Windows.Forms.TabPage();
            this.label43 = new System.Windows.Forms.Label();
            this.btn_reset = new System.Windows.Forms.Button();
            this.rtf_x264Query = new System.Windows.Forms.RichTextBox();
            this.lbl_trellis = new System.Windows.Forms.Label();
            this.label41 = new System.Windows.Forms.Label();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.label45 = new System.Windows.Forms.Label();
            this.label48 = new System.Windows.Forms.Label();
            this.label49 = new System.Windows.Forms.Label();
            this.label54 = new System.Windows.Forms.Label();
            this.lbl_direct_prediction = new System.Windows.Forms.Label();
            this.label62 = new System.Windows.Forms.Label();
            this.label64 = new System.Windows.Forms.Label();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.btn_clear = new System.Windows.Forms.Button();
            this.label34 = new System.Windows.Forms.Label();
            this.btn_generate_Query = new System.Windows.Forms.Button();
            this.label33 = new System.Windows.Forms.Label();
            this.rtf_query = new System.Windows.Forms.RichTextBox();
            this.groupBox_dest = new System.Windows.Forms.GroupBox();
            this.btn_destBrowse = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.treeView_presets = new System.Windows.Forms.TreeView();
            this.presets_menu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.pmnu_expandAll = new System.Windows.Forms.ToolStripMenuItem();
            this.pmnu_collapse = new System.Windows.Forms.ToolStripMenuItem();
            this.sep1 = new System.Windows.Forms.ToolStripSeparator();
            this.pmnu_delete = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.btn_source = new System.Windows.Forms.ToolStripDropDownButton();
            this.btn_file_source = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_dvd_source = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_dvd_drive = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator10 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_start = new System.Windows.Forms.ToolStripButton();
            this.btn_add2Queue = new System.Windows.Forms.ToolStripButton();
            this.btn_showQueue = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_ActivityWindow = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator8 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_minimize = new System.Windows.Forms.ToolStripButton();
            this.notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.StatusStrip = new System.Windows.Forms.StatusStrip();
            this.lbl_encode = new System.Windows.Forms.ToolStripStatusLabel();
            Label38 = new System.Windows.Forms.Label();
            notifyIconMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            notifyIconMenu.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.data_chpt)).BeginInit();
            this.frmMainMenu.SuspendLayout();
            this.GroupBox1.SuspendLayout();
            this.groupBox_output.SuspendLayout();
            this.TabPage2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar4)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar3)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
            this.TabPage3.SuspendLayout();
            this.TabPage1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.slider_deblock)).BeginInit();
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
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.presets_menu.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.StatusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // Label38
            // 
            Label38.AutoSize = true;
            Label38.BackColor = System.Drawing.Color.Transparent;
            Label38.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            Label38.Location = new System.Drawing.Point(334, 38);
            Label38.Name = "Label38";
            Label38.Size = new System.Drawing.Size(108, 13);
            Label38.TabIndex = 11;
            Label38.Text = "Target Size (MB):";
            // 
            // notifyIconMenu
            // 
            notifyIconMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_restore});
            notifyIconMenu.Name = "notifyIconMenu";
            notifyIconMenu.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            notifyIconMenu.Size = new System.Drawing.Size(129, 26);
            // 
            // btn_restore
            // 
            this.btn_restore.Name = "btn_restore";
            this.btn_restore.Size = new System.Drawing.Size(128, 22);
            this.btn_restore.Text = "Restore";
            this.btn_restore.Click += new System.EventHandler(this.btn_restore_Click);
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
            this.ToolTip.ToolTipTitle = "Tooltip";
            // 
            // drop_chapterFinish
            // 
            this.drop_chapterFinish.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_chapterFinish.FormattingEnabled = true;
            this.drop_chapterFinish.Location = new System.Drawing.Point(427, 52);
            this.drop_chapterFinish.Name = "drop_chapterFinish";
            this.drop_chapterFinish.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterFinish.TabIndex = 10;
            this.drop_chapterFinish.Text = "Auto";
            this.ToolTip.SetToolTip(this.drop_chapterFinish, "Select the chapter range you would like to enocde. (default: All Chapters)");
            this.drop_chapterFinish.SelectedIndexChanged += new System.EventHandler(this.drop_chapterFinish_SelectedIndexChanged);
            // 
            // drop_chapterStart
            // 
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
            this.drp_dvdtitle.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_dvdtitle.FormattingEnabled = true;
            this.drp_dvdtitle.Items.AddRange(new object[] {
            "Automatic"});
            this.drp_dvdtitle.Location = new System.Drawing.Point(99, 52);
            this.drp_dvdtitle.Name = "drp_dvdtitle";
            this.drp_dvdtitle.Size = new System.Drawing.Size(119, 21);
            this.drp_dvdtitle.TabIndex = 7;
            this.drp_dvdtitle.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_dvdtitle, "Select the title you wish to encode.\r\nThe longest title is selected by default af" +
                    "ter you have scanned a source.");
            this.drp_dvdtitle.SelectedIndexChanged += new System.EventHandler(this.drp_dvdtitle_SelectedIndexChanged);
            this.drp_dvdtitle.Click += new System.EventHandler(this.drp_dvdtitle_Click);
            // 
            // text_source
            // 
            this.text_source.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_source.Location = new System.Drawing.Point(99, 19);
            this.text_source.Name = "text_source";
            this.text_source.ReadOnly = true;
            this.text_source.Size = new System.Drawing.Size(584, 21);
            this.text_source.TabIndex = 1;
            this.text_source.Text = "Click \'Source\' to continue";
            this.ToolTip.SetToolTip(this.text_source, "Location of the source input file, folder or dvd.");
            // 
            // text_destination
            // 
            this.text_destination.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_destination.Location = new System.Drawing.Point(99, 19);
            this.text_destination.Name = "text_destination";
            this.text_destination.Size = new System.Drawing.Size(503, 21);
            this.text_destination.TabIndex = 1;
            this.ToolTip.SetToolTip(this.text_destination, "Location where the encoded file will be saved.");
            this.text_destination.TextChanged += new System.EventHandler(this.text_destination_TextChanged);
            // 
            // drp_videoEncoder
            // 
            this.drp_videoEncoder.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_videoEncoder.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_videoEncoder.FormattingEnabled = true;
            this.drp_videoEncoder.Items.AddRange(new object[] {
            "MPEG-4 (FFmpeg)",
            "MPEG-4 (XviD)",
            "H.264 (x264)",
            "VP3 (Theora)"});
            this.drp_videoEncoder.Location = new System.Drawing.Point(125, 35);
            this.drp_videoEncoder.Name = "drp_videoEncoder";
            this.drp_videoEncoder.Size = new System.Drawing.Size(126, 21);
            this.drp_videoEncoder.TabIndex = 1;
            this.ToolTip.SetToolTip(this.drp_videoEncoder, "Select a video encoder");
            this.drp_videoEncoder.SelectedIndexChanged += new System.EventHandler(this.drp_videoEncoder_SelectedIndexChanged);
            // 
            // drp_audbit_1
            // 
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
            this.drp_audbit_1.Location = new System.Drawing.Point(546, 53);
            this.drp_audbit_1.Name = "drp_audbit_1";
            this.drp_audbit_1.Size = new System.Drawing.Size(70, 20);
            this.drp_audbit_1.TabIndex = 11;
            this.drp_audbit_1.Text = "160";
            this.ToolTip.SetToolTip(this.drp_audbit_1, "Set the Audio Bit-Rate");
            // 
            // drp_audsr_1
            // 
            this.drp_audsr_1.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audsr_1.FormattingEnabled = true;
            this.drp_audsr_1.Items.AddRange(new object[] {
            "Auto",
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audsr_1.Location = new System.Drawing.Point(488, 53);
            this.drp_audsr_1.Name = "drp_audsr_1";
            this.drp_audsr_1.Size = new System.Drawing.Size(55, 20);
            this.drp_audsr_1.TabIndex = 9;
            this.drp_audsr_1.Text = "Auto";
            this.ToolTip.SetToolTip(this.drp_audsr_1, "Set the Audio Sample Rate");
            // 
            // check_largeFile
            // 
            this.check_largeFile.AutoSize = true;
            this.check_largeFile.BackColor = System.Drawing.Color.Transparent;
            this.check_largeFile.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_largeFile.Location = new System.Drawing.Point(193, 22);
            this.check_largeFile.Name = "check_largeFile";
            this.check_largeFile.Size = new System.Drawing.Size(105, 17);
            this.check_largeFile.TabIndex = 4;
            this.check_largeFile.Text = "Large file size";
            this.ToolTip.SetToolTip(this.check_largeFile, "Caution: This option will likely break device compatibility with all but the Appl" +
                    "eTV Take 2.\r\nChecking this box enables a 64bit mp4 file which can be over 4GB.");
            this.check_largeFile.UseVisualStyleBackColor = false;
            // 
            // check_turbo
            // 
            this.check_turbo.AutoSize = true;
            this.check_turbo.BackColor = System.Drawing.Color.Transparent;
            this.check_turbo.Enabled = false;
            this.check_turbo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_turbo.Location = new System.Drawing.Point(37, 184);
            this.check_turbo.Name = "check_turbo";
            this.check_turbo.Size = new System.Drawing.Size(115, 17);
            this.check_turbo.TabIndex = 7;
            this.check_turbo.Text = "Turbo first Pass";
            this.ToolTip.SetToolTip(this.check_turbo, "Makes the first pass of a 2 pass encode faster.");
            this.check_turbo.UseVisualStyleBackColor = false;
            // 
            // drp_videoFramerate
            // 
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
            this.drp_videoFramerate.Location = new System.Drawing.Point(125, 68);
            this.drp_videoFramerate.Name = "drp_videoFramerate";
            this.drp_videoFramerate.Size = new System.Drawing.Size(126, 21);
            this.drp_videoFramerate.TabIndex = 2;
            this.drp_videoFramerate.Text = "Same as source";
            this.ToolTip.SetToolTip(this.drp_videoFramerate, "Can be left to \"Same as source\" in most cases.");
            // 
            // slider_videoQuality
            // 
            this.slider_videoQuality.Location = new System.Drawing.Point(468, 91);
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
            this.text_filesize.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_filesize.Location = new System.Drawing.Point(476, 36);
            this.text_filesize.Name = "text_filesize";
            this.text_filesize.Size = new System.Drawing.Size(81, 21);
            this.text_filesize.TabIndex = 12;
            this.ToolTip.SetToolTip(this.text_filesize, "Set the file size you wish the encoded file to be.");
            this.text_filesize.TextChanged += new System.EventHandler(this.text_filesize_TextChanged);
            // 
            // text_bitrate
            // 
            this.text_bitrate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_bitrate.Location = new System.Drawing.Point(476, 63);
            this.text_bitrate.Name = "text_bitrate";
            this.text_bitrate.Size = new System.Drawing.Size(81, 21);
            this.text_bitrate.TabIndex = 10;
            this.ToolTip.SetToolTip(this.text_bitrate, "Set the bitrate of the video");
            this.text_bitrate.TextChanged += new System.EventHandler(this.text_bitrate_TextChanged);
            // 
            // drp_subtitle
            // 
            this.drp_subtitle.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_subtitle.FormattingEnabled = true;
            this.drp_subtitle.Items.AddRange(new object[] {
            "None",
            "Autoselect"});
            this.drp_subtitle.Location = new System.Drawing.Point(79, 244);
            this.drp_subtitle.Name = "drp_subtitle";
            this.drp_subtitle.Size = new System.Drawing.Size(138, 21);
            this.drp_subtitle.TabIndex = 43;
            this.drp_subtitle.Text = "None";
            this.ToolTip.SetToolTip(this.drp_subtitle, resources.GetString("drp_subtitle.ToolTip"));
            this.drp_subtitle.SelectedIndexChanged += new System.EventHandler(this.drp_subtitle_SelectedIndexChanged);
            // 
            // btn_setDefault
            // 
            this.btn_setDefault.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_setDefault.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_setDefault.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_setDefault.Location = new System.Drawing.Point(108, 8);
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
            this.drp_audmix_1.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audmix_1.FormattingEnabled = true;
            this.drp_audmix_1.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audmix_1.Location = new System.Drawing.Point(353, 53);
            this.drp_audmix_1.Name = "drp_audmix_1";
            this.drp_audmix_1.Size = new System.Drawing.Size(129, 20);
            this.drp_audmix_1.TabIndex = 7;
            this.drp_audmix_1.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_audmix_1, "Please note: Some options require a 5.1 audio channel to be selected");
            this.drp_audmix_1.SelectedIndexChanged += new System.EventHandler(this.drp_audmix_1_SelectedIndexChanged);
            // 
            // text_height
            // 
            this.text_height.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_height.ForeColor = System.Drawing.SystemColors.InfoText;
            this.text_height.Location = new System.Drawing.Point(498, 81);
            this.text_height.Name = "text_height";
            this.text_height.Size = new System.Drawing.Size(64, 21);
            this.text_height.TabIndex = 19;
            this.ToolTip.SetToolTip(this.text_height, "Video Resolution (Height)\r\nCan only be altered when Anamorphic is set to \"None\"");
            this.text_height.TextChanged += new System.EventHandler(this.text_height_TextChanged);
            // 
            // text_width
            // 
            this.text_width.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_width.Location = new System.Drawing.Point(407, 81);
            this.text_width.Name = "text_width";
            this.text_width.Size = new System.Drawing.Size(64, 21);
            this.text_width.TabIndex = 17;
            this.ToolTip.SetToolTip(this.text_width, "Video Resolution (Width)\r\nCan only be altered when Anamorphic is set to \"None\" or" +
                    " \"Loose\"");
            this.text_width.TextChanged += new System.EventHandler(this.text_width_TextChanged);
            // 
            // check_optimiseMP4
            // 
            this.check_optimiseMP4.AutoSize = true;
            this.check_optimiseMP4.BackColor = System.Drawing.Color.Transparent;
            this.check_optimiseMP4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_optimiseMP4.Location = new System.Drawing.Point(304, 22);
            this.check_optimiseMP4.Name = "check_optimiseMP4";
            this.check_optimiseMP4.Size = new System.Drawing.Size(110, 17);
            this.check_optimiseMP4.TabIndex = 25;
            this.check_optimiseMP4.Text = "Web optimized";
            this.ToolTip.SetToolTip(this.check_optimiseMP4, "MP4 files can be optimized for progressive downloads over the Web,\r\nbut note that" +
                    " QuickTime can only read the files as long as the file extension is .mp4\r\nCan on" +
                    "ly be used with H.264 ");
            this.check_optimiseMP4.UseVisualStyleBackColor = false;
            // 
            // check_iPodAtom
            // 
            this.check_iPodAtom.AutoSize = true;
            this.check_iPodAtom.BackColor = System.Drawing.Color.Transparent;
            this.check_iPodAtom.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_iPodAtom.Location = new System.Drawing.Point(420, 22);
            this.check_iPodAtom.Name = "check_iPodAtom";
            this.check_iPodAtom.Size = new System.Drawing.Size(117, 17);
            this.check_iPodAtom.TabIndex = 26;
            this.check_iPodAtom.Text = "iPod 5G support";
            this.ToolTip.SetToolTip(this.check_iPodAtom, "Support for legacy 5th Generation iPods.\r\nEncodes will not sync if this option is" +
                    " not enabled for H.264 encodes.");
            this.check_iPodAtom.UseVisualStyleBackColor = false;
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
            this.btn_addPreset.Location = new System.Drawing.Point(3, 8);
            this.btn_addPreset.Name = "btn_addPreset";
            this.btn_addPreset.Size = new System.Drawing.Size(35, 22);
            this.btn_addPreset.TabIndex = 3;
            this.btn_addPreset.TabStop = false;
            this.btn_addPreset.Text = "Add";
            this.ToolTip.SetToolTip(this.btn_addPreset, "Add a preset to the preset panel");
            this.btn_addPreset.UseVisualStyleBackColor = true;
            this.btn_addPreset.Click += new System.EventHandler(this.btn_addPreset_Click);
            // 
            // btn_removePreset
            // 
            this.btn_removePreset.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_removePreset.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_removePreset.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_removePreset.Location = new System.Drawing.Point(44, 8);
            this.btn_removePreset.Name = "btn_removePreset";
            this.btn_removePreset.Size = new System.Drawing.Size(58, 22);
            this.btn_removePreset.TabIndex = 4;
            this.btn_removePreset.TabStop = false;
            this.btn_removePreset.Text = "Remove";
            this.ToolTip.SetToolTip(this.btn_removePreset, "Remove a preset from the panel above.");
            this.btn_removePreset.UseVisualStyleBackColor = true;
            this.btn_removePreset.Click += new System.EventHandler(this.btn_removePreset_Click);
            // 
            // drp_audmix_2
            // 
            this.drp_audmix_2.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audmix_2.FormattingEnabled = true;
            this.drp_audmix_2.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audmix_2.Location = new System.Drawing.Point(353, 92);
            this.drp_audmix_2.Name = "drp_audmix_2";
            this.drp_audmix_2.Size = new System.Drawing.Size(129, 20);
            this.drp_audmix_2.TabIndex = 19;
            this.drp_audmix_2.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_audmix_2, "Please note: Some options require a 5.1 audio channel to be selected");
            this.drp_audmix_2.SelectedIndexChanged += new System.EventHandler(this.drp_audmix_2_SelectedIndexChanged);
            // 
            // drp_audenc_1
            // 
            this.drp_audenc_1.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audenc_1.FormattingEnabled = true;
            this.drp_audenc_1.Items.AddRange(new object[] {
            "AAC",
            "MP3",
            "Vorbis",
            "AC3"});
            this.drp_audenc_1.Location = new System.Drawing.Point(236, 53);
            this.drp_audenc_1.Name = "drp_audenc_1";
            this.drp_audenc_1.Size = new System.Drawing.Size(111, 20);
            this.drp_audenc_1.TabIndex = 5;
            this.drp_audenc_1.Text = "AAC";
            this.ToolTip.SetToolTip(this.drp_audenc_1, "Select an audio encoder.");
            this.drp_audenc_1.SelectedIndexChanged += new System.EventHandler(this.drp_audenc_1_SelectedIndexChanged);
            // 
            // drp_audenc_2
            // 
            this.drp_audenc_2.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audenc_2.FormattingEnabled = true;
            this.drp_audenc_2.Items.AddRange(new object[] {
            "AAC",
            "MP3",
            "Vorbis",
            "AC3"});
            this.drp_audenc_2.Location = new System.Drawing.Point(236, 92);
            this.drp_audenc_2.Name = "drp_audenc_2";
            this.drp_audenc_2.Size = new System.Drawing.Size(111, 20);
            this.drp_audenc_2.TabIndex = 18;
            this.drp_audenc_2.Text = "AAC";
            this.ToolTip.SetToolTip(this.drp_audenc_2, "Select an audio encoder.");
            this.drp_audenc_2.SelectedIndexChanged += new System.EventHandler(this.drp_audenc_2_SelectedIndexChanged);
            // 
            // drp_audbit_2
            // 
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
            this.drp_audbit_2.Location = new System.Drawing.Point(546, 92);
            this.drp_audbit_2.Name = "drp_audbit_2";
            this.drp_audbit_2.Size = new System.Drawing.Size(71, 20);
            this.drp_audbit_2.TabIndex = 21;
            this.drp_audbit_2.Text = "160";
            this.ToolTip.SetToolTip(this.drp_audbit_2, "Set the Audio Bit-Rate");
            // 
            // drp_audsr_2
            // 
            this.drp_audsr_2.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audsr_2.FormattingEnabled = true;
            this.drp_audsr_2.Items.AddRange(new object[] {
            "Auto",
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audsr_2.Location = new System.Drawing.Point(488, 92);
            this.drp_audsr_2.Name = "drp_audsr_2";
            this.drp_audsr_2.Size = new System.Drawing.Size(52, 20);
            this.drp_audsr_2.TabIndex = 20;
            this.drp_audsr_2.Text = "Auto";
            this.ToolTip.SetToolTip(this.drp_audsr_2, "Set the Audio Sample Rate");
            // 
            // drp_audsr_3
            // 
            this.drp_audsr_3.Enabled = false;
            this.drp_audsr_3.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audsr_3.FormattingEnabled = true;
            this.drp_audsr_3.Items.AddRange(new object[] {
            "Auto",
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audsr_3.Location = new System.Drawing.Point(488, 133);
            this.drp_audsr_3.Name = "drp_audsr_3";
            this.drp_audsr_3.Size = new System.Drawing.Size(52, 20);
            this.drp_audsr_3.TabIndex = 28;
            this.drp_audsr_3.Text = "Auto";
            this.ToolTip.SetToolTip(this.drp_audsr_3, "Set the Audio Sample Rate");
            // 
            // drp_audbit_3
            // 
            this.drp_audbit_3.Enabled = false;
            this.drp_audbit_3.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audbit_3.FormattingEnabled = true;
            this.drp_audbit_3.Items.AddRange(new object[] {
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
            this.drp_audbit_3.Location = new System.Drawing.Point(546, 133);
            this.drp_audbit_3.Name = "drp_audbit_3";
            this.drp_audbit_3.Size = new System.Drawing.Size(71, 20);
            this.drp_audbit_3.TabIndex = 29;
            this.drp_audbit_3.Text = "160";
            this.ToolTip.SetToolTip(this.drp_audbit_3, "Set the Audio Bit-Rate");
            // 
            // drp_audenc_3
            // 
            this.drp_audenc_3.Enabled = false;
            this.drp_audenc_3.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audenc_3.FormattingEnabled = true;
            this.drp_audenc_3.Items.AddRange(new object[] {
            "AAC",
            "MP3",
            "Vorbis",
            "AC3"});
            this.drp_audenc_3.Location = new System.Drawing.Point(236, 133);
            this.drp_audenc_3.Name = "drp_audenc_3";
            this.drp_audenc_3.Size = new System.Drawing.Size(111, 20);
            this.drp_audenc_3.TabIndex = 26;
            this.drp_audenc_3.Text = "AAC";
            this.ToolTip.SetToolTip(this.drp_audenc_3, "Select an audio encoder.");
            this.drp_audenc_3.SelectedIndexChanged += new System.EventHandler(this.drp_audenc_3_SelectedIndexChanged);
            // 
            // drp_audmix_3
            // 
            this.drp_audmix_3.Enabled = false;
            this.drp_audmix_3.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audmix_3.FormattingEnabled = true;
            this.drp_audmix_3.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audmix_3.Location = new System.Drawing.Point(353, 133);
            this.drp_audmix_3.Name = "drp_audmix_3";
            this.drp_audmix_3.Size = new System.Drawing.Size(129, 20);
            this.drp_audmix_3.TabIndex = 27;
            this.drp_audmix_3.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_audmix_3, "Please note: Some options require a 5.1 audio channel to be selected");
            this.drp_audmix_3.SelectedIndexChanged += new System.EventHandler(this.drp_audmix_3_SelectedIndexChanged);
            // 
            // drp_audsr_4
            // 
            this.drp_audsr_4.Enabled = false;
            this.drp_audsr_4.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audsr_4.FormattingEnabled = true;
            this.drp_audsr_4.Items.AddRange(new object[] {
            "Auto",
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audsr_4.Location = new System.Drawing.Point(488, 175);
            this.drp_audsr_4.Name = "drp_audsr_4";
            this.drp_audsr_4.Size = new System.Drawing.Size(52, 20);
            this.drp_audsr_4.TabIndex = 36;
            this.drp_audsr_4.Text = "Auto";
            this.ToolTip.SetToolTip(this.drp_audsr_4, "Set the Audio Sample Rate");
            // 
            // drp_audbit_4
            // 
            this.drp_audbit_4.Enabled = false;
            this.drp_audbit_4.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audbit_4.FormattingEnabled = true;
            this.drp_audbit_4.Items.AddRange(new object[] {
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
            this.drp_audbit_4.Location = new System.Drawing.Point(546, 175);
            this.drp_audbit_4.Name = "drp_audbit_4";
            this.drp_audbit_4.Size = new System.Drawing.Size(70, 20);
            this.drp_audbit_4.TabIndex = 37;
            this.drp_audbit_4.Text = "160";
            this.ToolTip.SetToolTip(this.drp_audbit_4, "Set the Audio Bit-Rate");
            // 
            // drp_audenc_4
            // 
            this.drp_audenc_4.Enabled = false;
            this.drp_audenc_4.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audenc_4.FormattingEnabled = true;
            this.drp_audenc_4.Items.AddRange(new object[] {
            "AAC",
            "MP3",
            "Vorbis",
            "AC3"});
            this.drp_audenc_4.Location = new System.Drawing.Point(236, 175);
            this.drp_audenc_4.Name = "drp_audenc_4";
            this.drp_audenc_4.Size = new System.Drawing.Size(111, 20);
            this.drp_audenc_4.TabIndex = 34;
            this.drp_audenc_4.Text = "AAC";
            this.ToolTip.SetToolTip(this.drp_audenc_4, "Select an audio encoder.");
            this.drp_audenc_4.SelectedIndexChanged += new System.EventHandler(this.drp_audenc_4_SelectedIndexChanged);
            // 
            // drp_audmix_4
            // 
            this.drp_audmix_4.Enabled = false;
            this.drp_audmix_4.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audmix_4.FormattingEnabled = true;
            this.drp_audmix_4.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audmix_4.Location = new System.Drawing.Point(353, 175);
            this.drp_audmix_4.Name = "drp_audmix_4";
            this.drp_audmix_4.Size = new System.Drawing.Size(129, 20);
            this.drp_audmix_4.TabIndex = 35;
            this.drp_audmix_4.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_audmix_4, "Please note: Some options require a 5.1 audio channel to be selected");
            this.drp_audmix_4.SelectedIndexChanged += new System.EventHandler(this.drp_audmix_4_SelectedIndexChanged);
            // 
            // drop_format
            // 
            this.drop_format.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_format.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_format.FormattingEnabled = true;
            this.drop_format.Items.AddRange(new object[] {
            "MP4 File",
            "M4V File",
            "MKV File",
            "AVI File",
            "OGM File"});
            this.drop_format.Location = new System.Drawing.Point(75, 19);
            this.drop_format.Name = "drop_format";
            this.drop_format.Size = new System.Drawing.Size(106, 21);
            this.drop_format.TabIndex = 28;
            this.ToolTip.SetToolTip(this.drop_format, "Select the file container format.");
            this.drop_format.SelectedIndexChanged += new System.EventHandler(this.drop_format_SelectedIndexChanged);
            // 
            // check_customCrop
            // 
            this.check_customCrop.AutoSize = true;
            this.check_customCrop.Location = new System.Drawing.Point(16, 58);
            this.check_customCrop.Name = "check_customCrop";
            this.check_customCrop.Size = new System.Drawing.Size(74, 17);
            this.check_customCrop.TabIndex = 34;
            this.check_customCrop.Text = "Custom:";
            this.ToolTip.SetToolTip(this.check_customCrop, "Set some custom cropping values.");
            this.check_customCrop.UseVisualStyleBackColor = true;
            this.check_customCrop.CheckedChanged += new System.EventHandler(this.check_customCrop_CheckedChanged);
            // 
            // check_autoCrop
            // 
            this.check_autoCrop.AutoSize = true;
            this.check_autoCrop.Checked = true;
            this.check_autoCrop.Location = new System.Drawing.Point(16, 34);
            this.check_autoCrop.Name = "check_autoCrop";
            this.check_autoCrop.Size = new System.Drawing.Size(82, 17);
            this.check_autoCrop.TabIndex = 33;
            this.check_autoCrop.TabStop = true;
            this.check_autoCrop.Text = "Automatic";
            this.ToolTip.SetToolTip(this.check_autoCrop, "Automatically set cropping values.");
            this.check_autoCrop.UseVisualStyleBackColor = true;
            this.check_autoCrop.CheckedChanged += new System.EventHandler(this.check_autoCrop_CheckedChanged);
            // 
            // check_Cabac
            // 
            this.check_Cabac.AutoSize = true;
            this.check_Cabac.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_Cabac.Checked = true;
            this.check_Cabac.CheckState = System.Windows.Forms.CheckState.Checked;
            this.check_Cabac.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_Cabac.Location = new System.Drawing.Point(536, 214);
            this.check_Cabac.Name = "check_Cabac";
            this.check_Cabac.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_Cabac.Size = new System.Drawing.Size(144, 16);
            this.check_Cabac.TabIndex = 40;
            this.check_Cabac.Text = "CABAC Entropy Coding:";
            this.check_Cabac.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_Cabac, resources.GetString("check_Cabac.ToolTip"));
            this.check_Cabac.UseVisualStyleBackColor = true;
            this.check_Cabac.CheckStateChanged += new System.EventHandler(this.check_Cabac_CheckedChanged);
            // 
            // check_noDCTDecimate
            // 
            this.check_noDCTDecimate.AutoSize = true;
            this.check_noDCTDecimate.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_noDCTDecimate.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_noDCTDecimate.Location = new System.Drawing.Point(562, 192);
            this.check_noDCTDecimate.Name = "check_noDCTDecimate";
            this.check_noDCTDecimate.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_noDCTDecimate.Size = new System.Drawing.Size(118, 16);
            this.check_noDCTDecimate.TabIndex = 39;
            this.check_noDCTDecimate.Text = "No DCT-Decimate:";
            this.check_noDCTDecimate.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_noDCTDecimate, "Only use this with constant quality encoding. \r\nIt increases quality but also bit" +
                    "rate/file size.");
            this.check_noDCTDecimate.UseVisualStyleBackColor = true;
            this.check_noDCTDecimate.CheckStateChanged += new System.EventHandler(this.check_noDCTDecimate_CheckedChanged);
            // 
            // check_noFastPSkip
            // 
            this.check_noFastPSkip.AutoSize = true;
            this.check_noFastPSkip.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_noFastPSkip.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_noFastPSkip.Location = new System.Drawing.Point(444, 192);
            this.check_noFastPSkip.Name = "check_noFastPSkip";
            this.check_noFastPSkip.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_noFastPSkip.Size = new System.Drawing.Size(103, 16);
            this.check_noFastPSkip.TabIndex = 38;
            this.check_noFastPSkip.Text = "No Fast-P-Skip:";
            this.check_noFastPSkip.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_noFastPSkip, "This can help with blocking on solid colors like blue skies, \r\nbut it also slows " +
                    "down the encode.");
            this.check_noFastPSkip.UseVisualStyleBackColor = true;
            this.check_noFastPSkip.CheckStateChanged += new System.EventHandler(this.check_noFastPSkip_CheckedChanged);
            // 
            // drop_trellis
            // 
            this.drop_trellis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_trellis.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_trellis.FormattingEnabled = true;
            this.drop_trellis.Items.AddRange(new object[] {
            "Default (0)",
            "0",
            "1",
            "2"});
            this.drop_trellis.Location = new System.Drawing.Point(537, 166);
            this.drop_trellis.Name = "drop_trellis";
            this.drop_trellis.Size = new System.Drawing.Size(143, 20);
            this.drop_trellis.TabIndex = 37;
            this.ToolTip.SetToolTip(this.drop_trellis, "Trellis fine-tunes how bitrate is doled out, so it can reduce file size/bitrate o" +
                    "r increase quality. \r\nA value of 2 forces it to be used more often than a value " +
                    "of 1.");
            this.drop_trellis.SelectedIndexChanged += new System.EventHandler(this.drop_trellis_SelectedIndexChanged);
            // 
            // drop_deblockBeta
            // 
            this.drop_deblockBeta.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_deblockBeta.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_deblockBeta.FormattingEnabled = true;
            this.drop_deblockBeta.Items.AddRange(new object[] {
            "Default (0)",
            "-6",
            "-5",
            "-4",
            "-3",
            "-2",
            "-1",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6"});
            this.drop_deblockBeta.Location = new System.Drawing.Point(611, 139);
            this.drop_deblockBeta.Name = "drop_deblockBeta";
            this.drop_deblockBeta.Size = new System.Drawing.Size(69, 20);
            this.drop_deblockBeta.TabIndex = 36;
            this.ToolTip.SetToolTip(this.drop_deblockBeta, resources.GetString("drop_deblockBeta.ToolTip"));
            this.drop_deblockBeta.SelectedIndexChanged += new System.EventHandler(this.drop_deblockBeta_SelectedIndexChanged);
            // 
            // drop_deblockAlpha
            // 
            this.drop_deblockAlpha.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_deblockAlpha.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_deblockAlpha.FormattingEnabled = true;
            this.drop_deblockAlpha.Items.AddRange(new object[] {
            "Default (0)",
            "-6",
            "-5",
            "-4",
            "-3",
            "-2",
            "-1",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6"});
            this.drop_deblockAlpha.Location = new System.Drawing.Point(537, 139);
            this.drop_deblockAlpha.Name = "drop_deblockAlpha";
            this.drop_deblockAlpha.Size = new System.Drawing.Size(68, 20);
            this.drop_deblockAlpha.TabIndex = 35;
            this.ToolTip.SetToolTip(this.drop_deblockAlpha, resources.GetString("drop_deblockAlpha.ToolTip"));
            this.drop_deblockAlpha.SelectedIndexChanged += new System.EventHandler(this.drop_deblockAlpha_SelectedIndexChanged);
            // 
            // check_8x8DCT
            // 
            this.check_8x8DCT.AutoSize = true;
            this.check_8x8DCT.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_8x8DCT.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_8x8DCT.Location = new System.Drawing.Point(608, 106);
            this.check_8x8DCT.Name = "check_8x8DCT";
            this.check_8x8DCT.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_8x8DCT.Size = new System.Drawing.Size(71, 16);
            this.check_8x8DCT.TabIndex = 34;
            this.check_8x8DCT.Text = "8x8 DCT:";
            this.check_8x8DCT.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_8x8DCT, resources.GetString("check_8x8DCT.ToolTip"));
            this.check_8x8DCT.UseVisualStyleBackColor = true;
            this.check_8x8DCT.CheckStateChanged += new System.EventHandler(this.check_8x8DCT_CheckedChanged);
            // 
            // drop_analysis
            // 
            this.drop_analysis.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_analysis.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_analysis.FormattingEnabled = true;
            this.drop_analysis.Items.AddRange(new object[] {
            "Default (some)",
            "None",
            "All"});
            this.drop_analysis.Location = new System.Drawing.Point(537, 105);
            this.drop_analysis.Name = "drop_analysis";
            this.drop_analysis.Size = new System.Drawing.Size(63, 20);
            this.drop_analysis.TabIndex = 33;
            this.ToolTip.SetToolTip(this.drop_analysis, resources.GetString("drop_analysis.ToolTip"));
            this.drop_analysis.SelectedIndexChanged += new System.EventHandler(this.drop_analysis_SelectedIndexChanged);
            // 
            // drop_subpixelMotionEstimation
            // 
            this.drop_subpixelMotionEstimation.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_subpixelMotionEstimation.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_subpixelMotionEstimation.FormattingEnabled = true;
            this.drop_subpixelMotionEstimation.Items.AddRange(new object[] {
            "Default (6)",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9"});
            this.drop_subpixelMotionEstimation.Location = new System.Drawing.Point(537, 69);
            this.drop_subpixelMotionEstimation.Name = "drop_subpixelMotionEstimation";
            this.drop_subpixelMotionEstimation.Size = new System.Drawing.Size(139, 20);
            this.drop_subpixelMotionEstimation.TabIndex = 32;
            this.ToolTip.SetToolTip(this.drop_subpixelMotionEstimation, resources.GetString("drop_subpixelMotionEstimation.ToolTip"));
            this.drop_subpixelMotionEstimation.SelectedIndexChanged += new System.EventHandler(this.drop_subpixelMotionEstimation_SelectedIndexChanged);
            // 
            // drop_MotionEstimationRange
            // 
            this.drop_MotionEstimationRange.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_MotionEstimationRange.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_MotionEstimationRange.FormattingEnabled = true;
            this.drop_MotionEstimationRange.Items.AddRange(new object[] {
            "Default (16)",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16",
            "17",
            "18",
            "19",
            "20",
            "21",
            "22",
            "23",
            "24",
            "25",
            "26",
            "27",
            "28",
            "29",
            "30",
            "31",
            "32",
            "33",
            "34",
            "35",
            "36",
            "37",
            "38",
            "39",
            "40",
            "41",
            "42",
            "43",
            "44",
            "45",
            "46",
            "47",
            "48",
            "49",
            "50",
            "51",
            "52",
            "53",
            "54",
            "55",
            "56",
            "57",
            "58",
            "59",
            "60",
            "61",
            "62",
            "63",
            "64"});
            this.drop_MotionEstimationRange.Location = new System.Drawing.Point(537, 40);
            this.drop_MotionEstimationRange.Name = "drop_MotionEstimationRange";
            this.drop_MotionEstimationRange.Size = new System.Drawing.Size(139, 20);
            this.drop_MotionEstimationRange.TabIndex = 31;
            this.ToolTip.SetToolTip(this.drop_MotionEstimationRange, resources.GetString("drop_MotionEstimationRange.ToolTip"));
            this.drop_MotionEstimationRange.SelectedIndexChanged += new System.EventHandler(this.drop_MotionEstimationRange_SelectedIndexChanged);
            // 
            // drop_MotionEstimationMethod
            // 
            this.drop_MotionEstimationMethod.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_MotionEstimationMethod.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_MotionEstimationMethod.FormattingEnabled = true;
            this.drop_MotionEstimationMethod.ItemHeight = 12;
            this.drop_MotionEstimationMethod.Items.AddRange(new object[] {
            "Default (Hexagon)",
            "Diamond",
            "Hexagon",
            "Uneven Multi-Hexagon",
            "Exhaustive"});
            this.drop_MotionEstimationMethod.Location = new System.Drawing.Point(537, 11);
            this.drop_MotionEstimationMethod.Name = "drop_MotionEstimationMethod";
            this.drop_MotionEstimationMethod.Size = new System.Drawing.Size(139, 20);
            this.drop_MotionEstimationMethod.TabIndex = 30;
            this.ToolTip.SetToolTip(this.drop_MotionEstimationMethod, resources.GetString("drop_MotionEstimationMethod.ToolTip"));
            this.drop_MotionEstimationMethod.SelectedIndexChanged += new System.EventHandler(this.drop_MotionEstimationMethod_SelectedIndexChanged);
            // 
            // check_pyrmidalBFrames
            // 
            this.check_pyrmidalBFrames.AutoSize = true;
            this.check_pyrmidalBFrames.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_pyrmidalBFrames.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_pyrmidalBFrames.Location = new System.Drawing.Point(46, 165);
            this.check_pyrmidalBFrames.Name = "check_pyrmidalBFrames";
            this.check_pyrmidalBFrames.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_pyrmidalBFrames.Size = new System.Drawing.Size(121, 16);
            this.check_pyrmidalBFrames.TabIndex = 17;
            this.check_pyrmidalBFrames.Text = "Pyrmidal B-Frames:";
            this.check_pyrmidalBFrames.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_pyrmidalBFrames, resources.GetString("check_pyrmidalBFrames.ToolTip"));
            this.check_pyrmidalBFrames.UseVisualStyleBackColor = true;
            this.check_pyrmidalBFrames.CheckStateChanged += new System.EventHandler(this.check_pyrmidalBFrames_CheckedChanged);
            // 
            // check_weightedBFrames
            // 
            this.check_weightedBFrames.AutoSize = true;
            this.check_weightedBFrames.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_weightedBFrames.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_weightedBFrames.Location = new System.Drawing.Point(44, 144);
            this.check_weightedBFrames.Name = "check_weightedBFrames";
            this.check_weightedBFrames.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_weightedBFrames.Size = new System.Drawing.Size(123, 16);
            this.check_weightedBFrames.TabIndex = 14;
            this.check_weightedBFrames.Text = "Weighted B-Frames:";
            this.ToolTip.SetToolTip(this.check_weightedBFrames, resources.GetString("check_weightedBFrames.ToolTip"));
            this.check_weightedBFrames.UseVisualStyleBackColor = true;
            this.check_weightedBFrames.CheckStateChanged += new System.EventHandler(this.check_weightedBFrames_CheckedChanged);
            // 
            // drop_directPrediction
            // 
            this.drop_directPrediction.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_directPrediction.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_directPrediction.FormattingEnabled = true;
            this.drop_directPrediction.Items.AddRange(new object[] {
            "Default (Spatial)",
            "None",
            "Spatial",
            "Temporal",
            "Automatic"});
            this.drop_directPrediction.Location = new System.Drawing.Point(157, 118);
            this.drop_directPrediction.Name = "drop_directPrediction";
            this.drop_directPrediction.Size = new System.Drawing.Size(121, 20);
            this.drop_directPrediction.TabIndex = 13;
            this.ToolTip.SetToolTip(this.drop_directPrediction, resources.GetString("drop_directPrediction.ToolTip"));
            this.drop_directPrediction.SelectedIndexChanged += new System.EventHandler(this.drop_directPrediction_SelectedIndexChanged);
            // 
            // drop_bFrames
            // 
            this.drop_bFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_bFrames.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_bFrames.FormattingEnabled = true;
            this.drop_bFrames.Items.AddRange(new object[] {
            "Default (0)",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16"});
            this.drop_bFrames.Location = new System.Drawing.Point(157, 91);
            this.drop_bFrames.Name = "drop_bFrames";
            this.drop_bFrames.Size = new System.Drawing.Size(121, 20);
            this.drop_bFrames.TabIndex = 12;
            this.ToolTip.SetToolTip(this.drop_bFrames, "Sane values are 1-6. \r\nB-Frames are smaller than other frames, so they let you pa" +
                    "ck in more quality at the same bitrate. \r\nUse more of them with animated materia" +
                    "l: 9-16. ");
            this.drop_bFrames.SelectedIndexChanged += new System.EventHandler(this.drop_bFrames_SelectedIndexChanged);
            // 
            // drop_refFrames
            // 
            this.drop_refFrames.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_refFrames.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.drop_refFrames.FormattingEnabled = true;
            this.drop_refFrames.Items.AddRange(new object[] {
            "Default (1)",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12",
            "13",
            "14",
            "15",
            "16"});
            this.drop_refFrames.Location = new System.Drawing.Point(157, 37);
            this.drop_refFrames.Name = "drop_refFrames";
            this.drop_refFrames.Size = new System.Drawing.Size(121, 20);
            this.drop_refFrames.TabIndex = 10;
            this.ToolTip.SetToolTip(this.drop_refFrames, "Sane values are 1-6. The more you add, the higher the quality — but the slower th" +
                    "e encode. ");
            this.drop_refFrames.SelectedIndexChanged += new System.EventHandler(this.drop_refFrames_SelectedIndexChanged);
            // 
            // check_mixedReferences
            // 
            this.check_mixedReferences.AutoSize = true;
            this.check_mixedReferences.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.check_mixedReferences.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.check_mixedReferences.Location = new System.Drawing.Point(54, 63);
            this.check_mixedReferences.Name = "check_mixedReferences";
            this.check_mixedReferences.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.check_mixedReferences.Size = new System.Drawing.Size(114, 16);
            this.check_mixedReferences.TabIndex = 11;
            this.check_mixedReferences.Text = "Mixed References:";
            this.check_mixedReferences.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.ToolTip.SetToolTip(this.check_mixedReferences, "With this on, different parts of a frame will refer back to different prior frame" +
                    "s, \r\ndepending on what\'s best for that part of the image. ");
            this.check_mixedReferences.UseVisualStyleBackColor = true;
            this.check_mixedReferences.CheckStateChanged += new System.EventHandler(this.check_mixedReferences_CheckedChanged);
            // 
            // lbl_src_res
            // 
            this.lbl_src_res.AutoSize = true;
            this.lbl_src_res.BackColor = System.Drawing.Color.Transparent;
            this.lbl_src_res.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_src_res.Location = new System.Drawing.Point(405, 34);
            this.lbl_src_res.Name = "lbl_src_res";
            this.lbl_src_res.Size = new System.Drawing.Size(72, 12);
            this.lbl_src_res.TabIndex = 13;
            this.lbl_src_res.Text = "Select a Title";
            // 
            // lbl_duration
            // 
            this.lbl_duration.AutoSize = true;
            this.lbl_duration.BackColor = System.Drawing.Color.Transparent;
            this.lbl_duration.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_duration.Location = new System.Drawing.Point(569, 56);
            this.lbl_duration.Name = "lbl_duration";
            this.lbl_duration.Size = new System.Drawing.Size(72, 12);
            this.lbl_duration.TabIndex = 43;
            this.lbl_duration.Text = "Select a Title";
            // 
            // label_duration
            // 
            this.label_duration.AutoSize = true;
            this.label_duration.BackColor = System.Drawing.Color.Transparent;
            this.label_duration.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label_duration.Location = new System.Drawing.Point(502, 55);
            this.label_duration.Name = "label_duration";
            this.label_duration.Size = new System.Drawing.Size(61, 13);
            this.label_duration.TabIndex = 42;
            this.label_duration.Text = "Duration:";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.BackColor = System.Drawing.Color.Transparent;
            this.label7.Location = new System.Drawing.Point(311, 34);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(52, 13);
            this.label7.TabIndex = 12;
            this.label7.Text = "Source:";
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
            this.ISO_Open.Filter = "All Files|*.*";
            this.ISO_Open.RestoreDirectory = true;
            this.ISO_Open.SupportMultiDottedExtensions = true;
            // 
            // FileToolStripMenuItem
            // 
            this.FileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_exit});
            this.FileToolStripMenuItem.Name = "FileToolStripMenuItem";
            this.FileToolStripMenuItem.Size = new System.Drawing.Size(38, 20);
            this.FileToolStripMenuItem.Text = "&File";
            // 
            // mnu_exit
            // 
            this.mnu_exit.Name = "mnu_exit";
            this.mnu_exit.Size = new System.Drawing.Size(106, 22);
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
            this.mnu_encodeLog,
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
            this.mnu_encode.Size = new System.Drawing.Size(203, 22);
            this.mnu_encode.Text = "Show Queue";
            this.mnu_encode.Click += new System.EventHandler(this.mnu_encode_Click);
            // 
            // mnu_encodeLog
            // 
            this.mnu_encodeLog.Image = global::Handbrake.Properties.Resources.ActivityWindow_small;
            this.mnu_encodeLog.Name = "mnu_encodeLog";
            this.mnu_encodeLog.Size = new System.Drawing.Size(203, 22);
            this.mnu_encodeLog.Text = "Activity Window";
            this.mnu_encodeLog.Click += new System.EventHandler(this.mnu_encodeLog_Click);
            // 
            // ToolStripSeparator5
            // 
            this.ToolStripSeparator5.Name = "ToolStripSeparator5";
            this.ToolStripSeparator5.Size = new System.Drawing.Size(200, 6);
            // 
            // mnu_options
            // 
            this.mnu_options.Image = global::Handbrake.Properties.Resources.Pref_Small;
            this.mnu_options.Name = "mnu_options";
            this.mnu_options.Size = new System.Drawing.Size(203, 22);
            this.mnu_options.Text = "Options";
            this.mnu_options.Click += new System.EventHandler(this.mnu_options_Click);
            // 
            // PresetsToolStripMenuItem
            // 
            this.PresetsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_presetReset,
            this.mnu_delete_preset,
            this.toolStripSeparator7,
            this.btn_new_preset,
            this.mnu_SelectDefault});
            this.PresetsToolStripMenuItem.Name = "PresetsToolStripMenuItem";
            this.PresetsToolStripMenuItem.Size = new System.Drawing.Size(61, 20);
            this.PresetsToolStripMenuItem.Text = "&Presets";
            // 
            // mnu_presetReset
            // 
            this.mnu_presetReset.Name = "mnu_presetReset";
            this.mnu_presetReset.Size = new System.Drawing.Size(215, 22);
            this.mnu_presetReset.Text = "Update Built-in Presets";
            this.mnu_presetReset.ToolTipText = "Resets all presets.";
            this.mnu_presetReset.Click += new System.EventHandler(this.mnu_presetReset_Click);
            // 
            // mnu_delete_preset
            // 
            this.mnu_delete_preset.Name = "mnu_delete_preset";
            this.mnu_delete_preset.Size = new System.Drawing.Size(215, 22);
            this.mnu_delete_preset.Text = "Delete Built-in Presets";
            this.mnu_delete_preset.Click += new System.EventHandler(this.mnu_delete_preset_Click);
            // 
            // toolStripSeparator7
            // 
            this.toolStripSeparator7.Name = "toolStripSeparator7";
            this.toolStripSeparator7.Size = new System.Drawing.Size(212, 6);
            // 
            // btn_new_preset
            // 
            this.btn_new_preset.Name = "btn_new_preset";
            this.btn_new_preset.Size = new System.Drawing.Size(215, 22);
            this.btn_new_preset.Text = "New Preset";
            this.btn_new_preset.Click += new System.EventHandler(this.btn_new_preset_Click);
            // 
            // mnu_SelectDefault
            // 
            this.mnu_SelectDefault.Name = "mnu_SelectDefault";
            this.mnu_SelectDefault.Size = new System.Drawing.Size(215, 22);
            this.mnu_SelectDefault.Text = "Select Default Preset";
            this.mnu_SelectDefault.ToolTipText = "Select HandBrake\'s default preset";
            this.mnu_SelectDefault.Click += new System.EventHandler(this.mnu_SelectDefault_Click);
            // 
            // HelpToolStripMenuItem
            // 
            this.HelpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_user_guide,
            this.mnu_handbrake_home,
            this.mnu_handbrake_forums,
            this.ToolStripSeparator3,
            this.mnu_UpdateCheck,
            this.toolStripSeparator6,
            this.mnu_about});
            this.HelpToolStripMenuItem.Name = "HelpToolStripMenuItem";
            this.HelpToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.HelpToolStripMenuItem.Text = "&Help";
            // 
            // mnu_user_guide
            // 
            this.mnu_user_guide.Image = global::Handbrake.Properties.Resources.Help16;
            this.mnu_user_guide.Name = "mnu_user_guide";
            this.mnu_user_guide.Size = new System.Drawing.Size(215, 22);
            this.mnu_user_guide.Text = "HandBrake User Guide";
            this.mnu_user_guide.Click += new System.EventHandler(this.mnu_user_guide_Click);
            // 
            // mnu_handbrake_home
            // 
            this.mnu_handbrake_home.Image = global::Handbrake.Properties.Resources.info16;
            this.mnu_handbrake_home.Name = "mnu_handbrake_home";
            this.mnu_handbrake_home.Size = new System.Drawing.Size(215, 22);
            this.mnu_handbrake_home.Text = "HandBrake Homepage";
            this.mnu_handbrake_home.Click += new System.EventHandler(this.mnu_handbrake_home_Click);
            // 
            // mnu_handbrake_forums
            // 
            this.mnu_handbrake_forums.Name = "mnu_handbrake_forums";
            this.mnu_handbrake_forums.Size = new System.Drawing.Size(215, 22);
            this.mnu_handbrake_forums.Text = "HandBrake Forums";
            this.mnu_handbrake_forums.Click += new System.EventHandler(this.mnu_handbrake_forums_Click);
            // 
            // ToolStripSeparator3
            // 
            this.ToolStripSeparator3.Name = "ToolStripSeparator3";
            this.ToolStripSeparator3.Size = new System.Drawing.Size(212, 6);
            // 
            // mnu_UpdateCheck
            // 
            this.mnu_UpdateCheck.Name = "mnu_UpdateCheck";
            this.mnu_UpdateCheck.Size = new System.Drawing.Size(215, 22);
            this.mnu_UpdateCheck.Text = "Check for Updates";
            this.mnu_UpdateCheck.Click += new System.EventHandler(this.mnu_UpdateCheck_Click);
            // 
            // toolStripSeparator6
            // 
            this.toolStripSeparator6.Name = "toolStripSeparator6";
            this.toolStripSeparator6.Size = new System.Drawing.Size(212, 6);
            // 
            // mnu_about
            // 
            this.mnu_about.Image = global::Handbrake.Properties.Resources.hb16;
            this.mnu_about.Name = "mnu_about";
            this.mnu_about.Size = new System.Drawing.Size(215, 22);
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
            this.frmMainMenu.Size = new System.Drawing.Size(985, 24);
            this.frmMainMenu.TabIndex = 0;
            this.frmMainMenu.Text = "MenuStrip1";
            // 
            // GroupBox1
            // 
            this.GroupBox1.Controls.Add(this.lbl_duration);
            this.GroupBox1.Controls.Add(this.label_duration);
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
            this.GroupBox1.Location = new System.Drawing.Point(12, 70);
            this.GroupBox1.Name = "GroupBox1";
            this.GroupBox1.Size = new System.Drawing.Size(705, 87);
            this.GroupBox1.TabIndex = 2;
            this.GroupBox1.TabStop = false;
            this.GroupBox1.Text = "Source";
            // 
            // Label13
            // 
            this.Label13.AutoSize = true;
            this.Label13.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label13.Location = new System.Drawing.Point(370, 55);
            this.Label13.Name = "Label13";
            this.Label13.Size = new System.Drawing.Size(51, 13);
            this.Label13.TabIndex = 10;
            this.Label13.Text = "through";
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
            this.groupBox_output.Controls.Add(this.drop_format);
            this.groupBox_output.Controls.Add(this.label5);
            this.groupBox_output.Controls.Add(this.check_largeFile);
            this.groupBox_output.Controls.Add(this.check_iPodAtom);
            this.groupBox_output.Controls.Add(this.check_optimiseMP4);
            this.groupBox_output.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox_output.ForeColor = System.Drawing.Color.Black;
            this.groupBox_output.Location = new System.Drawing.Point(12, 214);
            this.groupBox_output.Name = "groupBox_output";
            this.groupBox_output.Size = new System.Drawing.Size(705, 50);
            this.groupBox_output.TabIndex = 4;
            this.groupBox_output.TabStop = false;
            this.groupBox_output.Text = "Output Settings (Preset: None)";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.ForeColor = System.Drawing.Color.Black;
            this.label5.Location = new System.Drawing.Point(17, 23);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(52, 13);
            this.label5.TabIndex = 27;
            this.label5.Text = "Format:";
            // 
            // Label47
            // 
            this.Label47.AutoSize = true;
            this.Label47.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label47.ForeColor = System.Drawing.Color.Black;
            this.Label47.Location = new System.Drawing.Point(13, 39);
            this.Label47.Name = "Label47";
            this.Label47.Size = new System.Drawing.Size(84, 13);
            this.Label47.TabIndex = 0;
            this.Label47.Text = "Video Codec:";
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
            // TabPage2
            // 
            this.TabPage2.BackColor = System.Drawing.Color.Transparent;
            this.TabPage2.Controls.Add(this.lbl_drc4);
            this.TabPage2.Controls.Add(this.lbl_drc3);
            this.TabPage2.Controls.Add(this.lbl_drc2);
            this.TabPage2.Controls.Add(this.lbl_drc1);
            this.TabPage2.Controls.Add(this.trackBar4);
            this.TabPage2.Controls.Add(this.drp_audsr_4);
            this.TabPage2.Controls.Add(this.drp_audbit_4);
            this.TabPage2.Controls.Add(this.drp_audenc_4);
            this.TabPage2.Controls.Add(this.drp_audmix_4);
            this.TabPage2.Controls.Add(this.drp_track4Audio);
            this.TabPage2.Controls.Add(this.lbl_t4);
            this.TabPage2.Controls.Add(this.trackBar3);
            this.TabPage2.Controls.Add(this.drp_audsr_3);
            this.TabPage2.Controls.Add(this.drp_audbit_3);
            this.TabPage2.Controls.Add(this.drp_audenc_3);
            this.TabPage2.Controls.Add(this.drp_audmix_3);
            this.TabPage2.Controls.Add(this.drp_track3Audio);
            this.TabPage2.Controls.Add(this.lbl_t3);
            this.TabPage2.Controls.Add(this.trackBar2);
            this.TabPage2.Controls.Add(this.label16);
            this.TabPage2.Controls.Add(this.trackBar1);
            this.TabPage2.Controls.Add(this.groupBox5);
            this.TabPage2.Controls.Add(this.groupBox3);
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
            this.TabPage2.Controls.Add(this.Label32);
            this.TabPage2.Controls.Add(this.drp_audsr_1);
            this.TabPage2.Location = new System.Drawing.Point(4, 22);
            this.TabPage2.Name = "TabPage2";
            this.TabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage2.Size = new System.Drawing.Size(697, 316);
            this.TabPage2.TabIndex = 3;
            this.TabPage2.Text = "Audio && Subtitles";
            // 
            // lbl_drc4
            // 
            this.lbl_drc4.AutoSize = true;
            this.lbl_drc4.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drc4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_drc4.Location = new System.Drawing.Point(670, 176);
            this.lbl_drc4.Name = "lbl_drc4";
            this.lbl_drc4.Size = new System.Drawing.Size(19, 13);
            this.lbl_drc4.TabIndex = 39;
            this.lbl_drc4.Text = "1:";
            // 
            // lbl_drc3
            // 
            this.lbl_drc3.AutoSize = true;
            this.lbl_drc3.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drc3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_drc3.Location = new System.Drawing.Point(670, 135);
            this.lbl_drc3.Name = "lbl_drc3";
            this.lbl_drc3.Size = new System.Drawing.Size(19, 13);
            this.lbl_drc3.TabIndex = 31;
            this.lbl_drc3.Text = "1:";
            // 
            // lbl_drc2
            // 
            this.lbl_drc2.AutoSize = true;
            this.lbl_drc2.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drc2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_drc2.Location = new System.Drawing.Point(670, 90);
            this.lbl_drc2.Name = "lbl_drc2";
            this.lbl_drc2.Size = new System.Drawing.Size(19, 13);
            this.lbl_drc2.TabIndex = 23;
            this.lbl_drc2.Text = "1:";
            // 
            // lbl_drc1
            // 
            this.lbl_drc1.AutoSize = true;
            this.lbl_drc1.BackColor = System.Drawing.Color.Transparent;
            this.lbl_drc1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_drc1.Location = new System.Drawing.Point(670, 53);
            this.lbl_drc1.Name = "lbl_drc1";
            this.lbl_drc1.Size = new System.Drawing.Size(19, 13);
            this.lbl_drc1.TabIndex = 15;
            this.lbl_drc1.Text = "1:";
            // 
            // trackBar4
            // 
            this.trackBar4.Enabled = false;
            this.trackBar4.LargeChange = 0;
            this.trackBar4.Location = new System.Drawing.Point(620, 169);
            this.trackBar4.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar4.Maximum = 30;
            this.trackBar4.Name = "trackBar4";
            this.trackBar4.Size = new System.Drawing.Size(49, 42);
            this.trackBar4.TabIndex = 38;
            this.trackBar4.TickFrequency = 10;
            this.trackBar4.ValueChanged += new System.EventHandler(this.trackBar4_Scroll);
            // 
            // drp_track4Audio
            // 
            this.drp_track4Audio.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_track4Audio.FormattingEnabled = true;
            this.drp_track4Audio.Items.AddRange(new object[] {
            "None"});
            this.drp_track4Audio.Location = new System.Drawing.Point(36, 174);
            this.drp_track4Audio.Name = "drp_track4Audio";
            this.drp_track4Audio.Size = new System.Drawing.Size(194, 20);
            this.drp_track4Audio.TabIndex = 33;
            this.drp_track4Audio.SelectedIndexChanged += new System.EventHandler(this.drp_track4Audio_SelectedIndexChanged);
            // 
            // lbl_t4
            // 
            this.lbl_t4.AutoSize = true;
            this.lbl_t4.BackColor = System.Drawing.Color.Transparent;
            this.lbl_t4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_t4.Location = new System.Drawing.Point(13, 177);
            this.lbl_t4.Name = "lbl_t4";
            this.lbl_t4.Size = new System.Drawing.Size(19, 13);
            this.lbl_t4.TabIndex = 32;
            this.lbl_t4.Text = "4:";
            // 
            // trackBar3
            // 
            this.trackBar3.Enabled = false;
            this.trackBar3.LargeChange = 0;
            this.trackBar3.Location = new System.Drawing.Point(620, 128);
            this.trackBar3.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar3.Maximum = 30;
            this.trackBar3.Name = "trackBar3";
            this.trackBar3.Size = new System.Drawing.Size(49, 42);
            this.trackBar3.TabIndex = 30;
            this.trackBar3.TickFrequency = 10;
            this.trackBar3.ValueChanged += new System.EventHandler(this.trackBar3_Scroll);
            // 
            // drp_track3Audio
            // 
            this.drp_track3Audio.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_track3Audio.FormattingEnabled = true;
            this.drp_track3Audio.Items.AddRange(new object[] {
            "None"});
            this.drp_track3Audio.Location = new System.Drawing.Point(36, 132);
            this.drp_track3Audio.Name = "drp_track3Audio";
            this.drp_track3Audio.Size = new System.Drawing.Size(194, 20);
            this.drp_track3Audio.TabIndex = 25;
            this.drp_track3Audio.SelectedIndexChanged += new System.EventHandler(this.drp_track3Audio_SelectedIndexChanged);
            // 
            // lbl_t3
            // 
            this.lbl_t3.AutoSize = true;
            this.lbl_t3.BackColor = System.Drawing.Color.Transparent;
            this.lbl_t3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_t3.Location = new System.Drawing.Point(13, 135);
            this.lbl_t3.Name = "lbl_t3";
            this.lbl_t3.Size = new System.Drawing.Size(19, 13);
            this.lbl_t3.TabIndex = 24;
            this.lbl_t3.Text = "3:";
            // 
            // trackBar2
            // 
            this.trackBar2.LargeChange = 0;
            this.trackBar2.Location = new System.Drawing.Point(620, 86);
            this.trackBar2.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar2.Maximum = 30;
            this.trackBar2.Name = "trackBar2";
            this.trackBar2.Size = new System.Drawing.Size(49, 42);
            this.trackBar2.TabIndex = 22;
            this.trackBar2.TickFrequency = 10;
            this.trackBar2.ValueChanged += new System.EventHandler(this.trackBar2_Scroll);
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.BackColor = System.Drawing.Color.Transparent;
            this.label16.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label16.Location = new System.Drawing.Point(628, 36);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(28, 12);
            this.label16.TabIndex = 14;
            this.label16.Text = "DRC";
            // 
            // trackBar1
            // 
            this.trackBar1.LargeChange = 0;
            this.trackBar1.Location = new System.Drawing.Point(619, 48);
            this.trackBar1.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar1.Maximum = 30;
            this.trackBar1.Name = "trackBar1";
            this.trackBar1.Size = new System.Drawing.Size(50, 42);
            this.trackBar1.TabIndex = 13;
            this.trackBar1.TickFrequency = 10;
            this.trackBar1.ValueChanged += new System.EventHandler(this.trackBar1_Scroll);
            // 
            // groupBox5
            // 
            this.groupBox5.Location = new System.Drawing.Point(107, 13);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(577, 10);
            this.groupBox5.TabIndex = 1;
            this.groupBox5.TabStop = false;
            // 
            // groupBox3
            // 
            this.groupBox3.Location = new System.Drawing.Point(83, 219);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(601, 10);
            this.groupBox3.TabIndex = 41;
            this.groupBox3.TabStop = false;
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
            this.label67.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label67.Location = new System.Drawing.Point(479, 24);
            this.label67.Name = "label67";
            this.label67.Size = new System.Drawing.Size(65, 24);
            this.label67.TabIndex = 10;
            this.label67.Text = "Samplerate \r\n(kHz)";
            // 
            // label66
            // 
            this.label66.AutoSize = true;
            this.label66.BackColor = System.Drawing.Color.Transparent;
            this.label66.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label66.Location = new System.Drawing.Point(542, 36);
            this.label66.Name = "label66";
            this.label66.Size = new System.Drawing.Size(75, 12);
            this.label66.TabIndex = 12;
            this.label66.Text = "Bitrate (Kbps)";
            // 
            // label65
            // 
            this.label65.AutoSize = true;
            this.label65.BackColor = System.Drawing.Color.Transparent;
            this.label65.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label65.Location = new System.Drawing.Point(255, 36);
            this.label65.Name = "label65";
            this.label65.Size = new System.Drawing.Size(69, 12);
            this.label65.TabIndex = 6;
            this.label65.Text = "Audio Codec";
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.BackColor = System.Drawing.Color.Transparent;
            this.label14.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label14.Location = new System.Drawing.Point(380, 36);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(48, 12);
            this.label14.TabIndex = 8;
            this.label14.Text = "Mixdown";
            // 
            // check_forced
            // 
            this.check_forced.AutoSize = true;
            this.check_forced.BackColor = System.Drawing.Color.Transparent;
            this.check_forced.Enabled = false;
            this.check_forced.Location = new System.Drawing.Point(223, 247);
            this.check_forced.Name = "check_forced";
            this.check_forced.Size = new System.Drawing.Size(147, 17);
            this.check_forced.TabIndex = 44;
            this.check_forced.Text = "Forced Subtitles Only";
            this.check_forced.UseVisualStyleBackColor = false;
            // 
            // drp_track2Audio
            // 
            this.drp_track2Audio.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_track2Audio.FormattingEnabled = true;
            this.drp_track2Audio.Items.AddRange(new object[] {
            "None"});
            this.drp_track2Audio.Location = new System.Drawing.Point(36, 91);
            this.drp_track2Audio.Name = "drp_track2Audio";
            this.drp_track2Audio.Size = new System.Drawing.Size(194, 20);
            this.drp_track2Audio.TabIndex = 17;
            this.drp_track2Audio.SelectedIndexChanged += new System.EventHandler(this.drp_track2Audio_SelectedIndexChanged);
            // 
            // label28
            // 
            this.label28.AutoSize = true;
            this.label28.BackColor = System.Drawing.Color.Transparent;
            this.label28.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label28.Location = new System.Drawing.Point(13, 94);
            this.label28.Name = "label28";
            this.label28.Size = new System.Drawing.Size(19, 13);
            this.label28.TabIndex = 16;
            this.label28.Text = "2:";
            // 
            // label27
            // 
            this.label27.AutoSize = true;
            this.label27.BackColor = System.Drawing.Color.Transparent;
            this.label27.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label27.Location = new System.Drawing.Point(107, 36);
            this.label27.Name = "label27";
            this.label27.Size = new System.Drawing.Size(40, 12);
            this.label27.TabIndex = 4;
            this.label27.Text = "Source";
            // 
            // Label19
            // 
            this.Label19.AutoSize = true;
            this.Label19.BackColor = System.Drawing.Color.Transparent;
            this.Label19.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label19.Location = new System.Drawing.Point(13, 219);
            this.Label19.Name = "Label19";
            this.Label19.Size = new System.Drawing.Size(64, 13);
            this.Label19.TabIndex = 40;
            this.Label19.Text = "Subtitles";
            // 
            // Label20
            // 
            this.Label20.AutoSize = true;
            this.Label20.BackColor = System.Drawing.Color.Transparent;
            this.Label20.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label20.Location = new System.Drawing.Point(13, 247);
            this.Label20.Name = "Label20";
            this.Label20.Size = new System.Drawing.Size(61, 13);
            this.Label20.TabIndex = 42;
            this.Label20.Text = "Subtitles:";
            // 
            // drp_track1Audio
            // 
            this.drp_track1Audio.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_track1Audio.FormattingEnabled = true;
            this.drp_track1Audio.Items.AddRange(new object[] {
            "Automatic"});
            this.drp_track1Audio.Location = new System.Drawing.Point(36, 54);
            this.drp_track1Audio.Name = "drp_track1Audio";
            this.drp_track1Audio.Size = new System.Drawing.Size(194, 20);
            this.drp_track1Audio.TabIndex = 3;
            // 
            // Label32
            // 
            this.Label32.AutoSize = true;
            this.Label32.BackColor = System.Drawing.Color.Transparent;
            this.Label32.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label32.Location = new System.Drawing.Point(13, 57);
            this.Label32.Name = "Label32";
            this.Label32.Size = new System.Drawing.Size(19, 13);
            this.Label32.TabIndex = 2;
            this.Label32.Text = "1:";
            // 
            // TabPage3
            // 
            this.TabPage3.BackColor = System.Drawing.Color.Transparent;
            this.TabPage3.Controls.Add(this.drp_videoEncoder);
            this.TabPage3.Controls.Add(this.Label47);
            this.TabPage3.Controls.Add(this.label25);
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
            this.TabPage3.Size = new System.Drawing.Size(697, 316);
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
            this.label25.Size = new System.Drawing.Size(43, 13);
            this.label25.TabIndex = 0;
            this.label25.Text = "Video";
            // 
            // check_grayscale
            // 
            this.check_grayscale.AutoSize = true;
            this.check_grayscale.BackColor = System.Drawing.Color.Transparent;
            this.check_grayscale.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_grayscale.Location = new System.Drawing.Point(16, 138);
            this.check_grayscale.Name = "check_grayscale";
            this.check_grayscale.Size = new System.Drawing.Size(138, 17);
            this.check_grayscale.TabIndex = 5;
            this.check_grayscale.Text = "Grayscale Encoding";
            this.check_grayscale.UseVisualStyleBackColor = false;
            // 
            // Label22
            // 
            this.Label22.AutoSize = true;
            this.Label22.BackColor = System.Drawing.Color.Transparent;
            this.Label22.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label22.Location = new System.Drawing.Point(13, 118);
            this.Label22.Name = "Label22";
            this.Label22.Size = new System.Drawing.Size(191, 13);
            this.Label22.TabIndex = 4;
            this.Label22.Text = "Advanced Encoding Settings";
            // 
            // check_2PassEncode
            // 
            this.check_2PassEncode.AutoSize = true;
            this.check_2PassEncode.BackColor = System.Drawing.Color.Transparent;
            this.check_2PassEncode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_2PassEncode.Location = new System.Drawing.Point(16, 161);
            this.check_2PassEncode.Name = "check_2PassEncode";
            this.check_2PassEncode.Size = new System.Drawing.Size(119, 17);
            this.check_2PassEncode.TabIndex = 6;
            this.check_2PassEncode.Text = "2-Pass Encoding";
            this.check_2PassEncode.UseVisualStyleBackColor = false;
            this.check_2PassEncode.CheckedChanged += new System.EventHandler(this.check_2PassEncode_CheckedChanged);
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.BackColor = System.Drawing.Color.Transparent;
            this.Label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(334, 13);
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
            this.Label42.Location = new System.Drawing.Point(334, 65);
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
            this.SliderValue.Location = new System.Drawing.Point(641, 100);
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
            this.Label46.Location = new System.Drawing.Point(13, 71);
            this.Label46.Name = "Label46";
            this.Label46.Size = new System.Drawing.Size(106, 13);
            this.Label46.TabIndex = 1;
            this.Label46.Text = "Framerate (FPS):";
            // 
            // Label40
            // 
            this.Label40.AutoSize = true;
            this.Label40.BackColor = System.Drawing.Color.Transparent;
            this.Label40.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label40.Location = new System.Drawing.Point(334, 99);
            this.Label40.Name = "Label40";
            this.Label40.Size = new System.Drawing.Size(107, 13);
            this.Label40.TabIndex = 13;
            this.Label40.Text = "Constant Quality:";
            // 
            // TabPage1
            // 
            this.TabPage1.BackColor = System.Drawing.Color.Transparent;
            this.TabPage1.Controls.Add(this.slider_deblock);
            this.TabPage1.Controls.Add(this.label8);
            this.TabPage1.Controls.Add(this.lbl_deblockVal);
            this.TabPage1.Controls.Add(this.check_customCrop);
            this.TabPage1.Controls.Add(this.check_autoCrop);
            this.TabPage1.Controls.Add(this.check_decomb);
            this.TabPage1.Controls.Add(this.lbl_src_res);
            this.TabPage1.Controls.Add(this.label7);
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
            this.TabPage1.Controls.Add(this.label24);
            this.TabPage1.Controls.Add(this.drp_deNoise);
            this.TabPage1.Controls.Add(this.label11);
            this.TabPage1.Controls.Add(this.check_detelecine);
            this.TabPage1.Controls.Add(this.label4);
            this.TabPage1.Controls.Add(this.drp_deInterlace_option);
            this.TabPage1.Controls.Add(this.Label1);
            this.TabPage1.Controls.Add(this.Label53);
            this.TabPage1.Controls.Add(this.Label52);
            this.TabPage1.Controls.Add(this.Label51);
            this.TabPage1.Controls.Add(this.Label15);
            this.TabPage1.Location = new System.Drawing.Point(4, 22);
            this.TabPage1.Name = "TabPage1";
            this.TabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage1.Size = new System.Drawing.Size(697, 316);
            this.TabPage1.TabIndex = 0;
            this.TabPage1.Text = "Picture Settings";
            // 
            // slider_deblock
            // 
            this.slider_deblock.Location = new System.Drawing.Point(407, 264);
            this.slider_deblock.Maximum = 15;
            this.slider_deblock.Minimum = 4;
            this.slider_deblock.Name = "slider_deblock";
            this.slider_deblock.Size = new System.Drawing.Size(174, 42);
            this.slider_deblock.TabIndex = 35;
            this.slider_deblock.Value = 4;
            this.slider_deblock.Scroll += new System.EventHandler(this.slider_deblock_Scroll);
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.BackColor = System.Drawing.Color.Transparent;
            this.label8.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(311, 269);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(58, 13);
            this.label8.TabIndex = 37;
            this.label8.Text = "Deblock:";
            // 
            // lbl_deblockVal
            // 
            this.lbl_deblockVal.AutoSize = true;
            this.lbl_deblockVal.BackColor = System.Drawing.Color.Transparent;
            this.lbl_deblockVal.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_deblockVal.Location = new System.Drawing.Point(585, 269);
            this.lbl_deblockVal.Name = "lbl_deblockVal";
            this.lbl_deblockVal.Size = new System.Drawing.Size(24, 13);
            this.lbl_deblockVal.TabIndex = 36;
            this.lbl_deblockVal.Text = "Off";
            // 
            // check_decomb
            // 
            this.check_decomb.AutoSize = true;
            this.check_decomb.BackColor = System.Drawing.Color.Transparent;
            this.check_decomb.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_decomb.Location = new System.Drawing.Point(314, 188);
            this.check_decomb.Name = "check_decomb";
            this.check_decomb.Size = new System.Drawing.Size(73, 17);
            this.check_decomb.TabIndex = 32;
            this.check_decomb.Text = "Decomb";
            this.check_decomb.UseVisualStyleBackColor = false;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.BackColor = System.Drawing.Color.Transparent;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(311, 111);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(80, 13);
            this.label6.TabIndex = 20;
            this.label6.Text = "Anamorphic:";
            // 
            // drp_anamorphic
            // 
            this.drp_anamorphic.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_anamorphic.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_anamorphic.FormattingEnabled = true;
            this.drp_anamorphic.Items.AddRange(new object[] {
            "None",
            "Strict",
            "Loose"});
            this.drp_anamorphic.Location = new System.Drawing.Point(407, 108);
            this.drp_anamorphic.Name = "drp_anamorphic";
            this.drp_anamorphic.Size = new System.Drawing.Size(110, 21);
            this.drp_anamorphic.TabIndex = 21;
            this.drp_anamorphic.SelectedIndexChanged += new System.EventHandler(this.drp_anamorphic_SelectedIndexChanged);
            // 
            // text_bottom
            // 
            this.text_bottom.Enabled = false;
            this.text_bottom.Location = new System.Drawing.Point(96, 147);
            this.text_bottom.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.text_bottom.Name = "text_bottom";
            this.text_bottom.Size = new System.Drawing.Size(44, 21);
            this.text_bottom.TabIndex = 9;
            // 
            // text_top
            // 
            this.text_top.Enabled = false;
            this.text_top.Location = new System.Drawing.Point(96, 101);
            this.text_top.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.text_top.Name = "text_top";
            this.text_top.Size = new System.Drawing.Size(44, 21);
            this.text_top.TabIndex = 6;
            // 
            // text_left
            // 
            this.text_left.Enabled = false;
            this.text_left.Location = new System.Drawing.Point(45, 123);
            this.text_left.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.text_left.Name = "text_left";
            this.text_left.Size = new System.Drawing.Size(44, 21);
            this.text_left.TabIndex = 4;
            // 
            // text_right
            // 
            this.text_right.Enabled = false;
            this.text_right.Location = new System.Drawing.Point(147, 123);
            this.text_right.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.text_right.Name = "text_right";
            this.text_right.Size = new System.Drawing.Size(44, 21);
            this.text_right.TabIndex = 7;
            // 
            // label26
            // 
            this.label26.AutoSize = true;
            this.label26.BackColor = System.Drawing.Color.Transparent;
            this.label26.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label26.Location = new System.Drawing.Point(311, 13);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(34, 13);
            this.label26.TabIndex = 11;
            this.label26.Text = "Size";
            // 
            // Label56
            // 
            this.Label56.AutoSize = true;
            this.Label56.BackColor = System.Drawing.Color.Transparent;
            this.Label56.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label56.ForeColor = System.Drawing.Color.Black;
            this.Label56.Location = new System.Drawing.Point(477, 85);
            this.Label56.Name = "Label56";
            this.Label56.Size = new System.Drawing.Size(15, 13);
            this.Label56.TabIndex = 18;
            this.Label56.Text = "x";
            // 
            // lbl_Aspect
            // 
            this.lbl_Aspect.AutoSize = true;
            this.lbl_Aspect.BackColor = System.Drawing.Color.Transparent;
            this.lbl_Aspect.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_Aspect.Location = new System.Drawing.Point(405, 58);
            this.lbl_Aspect.Name = "lbl_Aspect";
            this.lbl_Aspect.Size = new System.Drawing.Size(72, 12);
            this.lbl_Aspect.TabIndex = 15;
            this.lbl_Aspect.Text = "Select a Title";
            // 
            // Label91
            // 
            this.Label91.AutoSize = true;
            this.Label91.BackColor = System.Drawing.Color.Transparent;
            this.Label91.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label91.Location = new System.Drawing.Point(311, 57);
            this.Label91.Name = "Label91";
            this.Label91.Size = new System.Drawing.Size(83, 13);
            this.Label91.TabIndex = 14;
            this.Label91.Text = "Aspect Ratio:";
            // 
            // Label55
            // 
            this.Label55.AutoSize = true;
            this.Label55.BackColor = System.Drawing.Color.Transparent;
            this.Label55.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label55.ForeColor = System.Drawing.Color.Black;
            this.Label55.Location = new System.Drawing.Point(311, 84);
            this.Label55.Name = "Label55";
            this.Label55.Size = new System.Drawing.Size(85, 13);
            this.Label55.TabIndex = 16;
            this.Label55.Text = "Width/Height:";
            // 
            // label24
            // 
            this.label24.AutoSize = true;
            this.label24.BackColor = System.Drawing.Color.Transparent;
            this.label24.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label24.Location = new System.Drawing.Point(311, 142);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(49, 13);
            this.label24.TabIndex = 22;
            this.label24.Text = "Filters";
            // 
            // drp_deNoise
            // 
            this.drp_deNoise.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_deNoise.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_deNoise.FormattingEnabled = true;
            this.drp_deNoise.Items.AddRange(new object[] {
            "None",
            "Weak",
            "Medium",
            "Strong"});
            this.drp_deNoise.Location = new System.Drawing.Point(413, 237);
            this.drp_deNoise.Name = "drp_deNoise";
            this.drp_deNoise.Size = new System.Drawing.Size(161, 21);
            this.drp_deNoise.TabIndex = 29;
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.BackColor = System.Drawing.Color.Transparent;
            this.label11.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(311, 240);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(58, 13);
            this.label11.TabIndex = 28;
            this.label11.Text = "Denoise:";
            // 
            // check_detelecine
            // 
            this.check_detelecine.AutoSize = true;
            this.check_detelecine.BackColor = System.Drawing.Color.Transparent;
            this.check_detelecine.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_detelecine.Location = new System.Drawing.Point(314, 165);
            this.check_detelecine.Name = "check_detelecine";
            this.check_detelecine.Size = new System.Drawing.Size(86, 17);
            this.check_detelecine.TabIndex = 23;
            this.check_detelecine.Text = "Detelecine";
            this.check_detelecine.UseVisualStyleBackColor = false;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.BackColor = System.Drawing.Color.Transparent;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(311, 212);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(77, 13);
            this.label4.TabIndex = 26;
            this.label4.Text = "Deinterlace:";
            // 
            // drp_deInterlace_option
            // 
            this.drp_deInterlace_option.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_deInterlace_option.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_deInterlace_option.FormattingEnabled = true;
            this.drp_deInterlace_option.Items.AddRange(new object[] {
            "None",
            "Fast",
            "Slow",
            "Slower"});
            this.drp_deInterlace_option.Location = new System.Drawing.Point(413, 209);
            this.drp_deInterlace_option.Name = "drp_deInterlace_option";
            this.drp_deInterlace_option.Size = new System.Drawing.Size(161, 21);
            this.drp_deInterlace_option.TabIndex = 27;
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
            this.Label53.Location = new System.Drawing.Point(94, 171);
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
            this.Label52.Location = new System.Drawing.Point(103, 86);
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
            this.Label51.Location = new System.Drawing.Point(190, 125);
            this.Label51.Name = "Label51";
            this.Label51.Size = new System.Drawing.Size(36, 13);
            this.Label51.TabIndex = 8;
            this.Label51.Text = "Right";
            // 
            // Label15
            // 
            this.Label15.AutoSize = true;
            this.Label15.BackColor = System.Drawing.Color.Transparent;
            this.Label15.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label15.Location = new System.Drawing.Point(13, 125);
            this.Label15.Name = "Label15";
            this.Label15.Size = new System.Drawing.Size(28, 13);
            this.Label15.TabIndex = 3;
            this.Label15.Text = "Left";
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
            this.advancedOptions.Location = new System.Drawing.Point(12, 274);
            this.advancedOptions.Name = "advancedOptions";
            this.advancedOptions.SelectedIndex = 0;
            this.advancedOptions.Size = new System.Drawing.Size(705, 342);
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
            this.tab_chapters.Size = new System.Drawing.Size(697, 316);
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
            this.h264Tab.Controls.Add(this.check_Cabac);
            this.h264Tab.Controls.Add(this.check_noDCTDecimate);
            this.h264Tab.Controls.Add(this.check_noFastPSkip);
            this.h264Tab.Controls.Add(this.lbl_trellis);
            this.h264Tab.Controls.Add(this.drop_trellis);
            this.h264Tab.Controls.Add(this.drop_deblockBeta);
            this.h264Tab.Controls.Add(this.label41);
            this.h264Tab.Controls.Add(this.drop_deblockAlpha);
            this.h264Tab.Controls.Add(this.panel3);
            this.h264Tab.Controls.Add(this.panel1);
            this.h264Tab.Controls.Add(this.panel2);
            this.h264Tab.Controls.Add(this.check_8x8DCT);
            this.h264Tab.Controls.Add(this.label45);
            this.h264Tab.Controls.Add(this.drop_analysis);
            this.h264Tab.Controls.Add(this.label48);
            this.h264Tab.Controls.Add(this.drop_subpixelMotionEstimation);
            this.h264Tab.Controls.Add(this.label49);
            this.h264Tab.Controls.Add(this.drop_MotionEstimationRange);
            this.h264Tab.Controls.Add(this.label54);
            this.h264Tab.Controls.Add(this.drop_MotionEstimationMethod);
            this.h264Tab.Controls.Add(this.check_pyrmidalBFrames);
            this.h264Tab.Controls.Add(this.check_weightedBFrames);
            this.h264Tab.Controls.Add(this.lbl_direct_prediction);
            this.h264Tab.Controls.Add(this.drop_directPrediction);
            this.h264Tab.Controls.Add(this.label62);
            this.h264Tab.Controls.Add(this.drop_bFrames);
            this.h264Tab.Controls.Add(this.label64);
            this.h264Tab.Controls.Add(this.drop_refFrames);
            this.h264Tab.Controls.Add(this.check_mixedReferences);
            this.h264Tab.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.h264Tab.Location = new System.Drawing.Point(4, 22);
            this.h264Tab.Name = "h264Tab";
            this.h264Tab.Padding = new System.Windows.Forms.Padding(3);
            this.h264Tab.Size = new System.Drawing.Size(697, 316);
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
            this.btn_reset.Location = new System.Drawing.Point(13, 224);
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
            // lbl_trellis
            // 
            this.lbl_trellis.AutoSize = true;
            this.lbl_trellis.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_trellis.Location = new System.Drawing.Point(490, 169);
            this.lbl_trellis.Name = "lbl_trellis";
            this.lbl_trellis.Size = new System.Drawing.Size(41, 12);
            this.lbl_trellis.TabIndex = 26;
            this.lbl_trellis.Text = "Trellis:";
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
            // lbl_direct_prediction
            // 
            this.lbl_direct_prediction.AutoSize = true;
            this.lbl_direct_prediction.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_direct_prediction.Location = new System.Drawing.Point(52, 121);
            this.lbl_direct_prediction.Name = "lbl_direct_prediction";
            this.lbl_direct_prediction.Size = new System.Drawing.Size(94, 12);
            this.lbl_direct_prediction.TabIndex = 5;
            this.lbl_direct_prediction.Text = "Direct Prediction:";
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
            // tabPage4
            // 
            this.tabPage4.Controls.Add(this.btn_clear);
            this.tabPage4.Controls.Add(this.label34);
            this.tabPage4.Controls.Add(this.btn_generate_Query);
            this.tabPage4.Controls.Add(this.label33);
            this.tabPage4.Controls.Add(this.rtf_query);
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Size = new System.Drawing.Size(697, 316);
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
            this.groupBox_dest.Location = new System.Drawing.Point(12, 160);
            this.groupBox_dest.Name = "groupBox_dest";
            this.groupBox_dest.Size = new System.Drawing.Size(705, 50);
            this.groupBox_dest.TabIndex = 3;
            this.groupBox_dest.TabStop = false;
            this.groupBox_dest.Text = "Destination";
            // 
            // btn_destBrowse
            // 
            this.btn_destBrowse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_destBrowse.Location = new System.Drawing.Point(608, 17);
            this.btn_destBrowse.Name = "btn_destBrowse";
            this.btn_destBrowse.Size = new System.Drawing.Size(75, 23);
            this.btn_destBrowse.TabIndex = 13;
            this.btn_destBrowse.Text = "Browse";
            this.btn_destBrowse.UseVisualStyleBackColor = true;
            this.btn_destBrowse.Click += new System.EventHandler(this.btn_destBrowse_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.splitContainer1);
            this.groupBox2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox2.ForeColor = System.Drawing.Color.Black;
            this.groupBox2.Location = new System.Drawing.Point(728, 70);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(245, 546);
            this.groupBox2.TabIndex = 6;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Presets";
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(3, 17);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.treeView_presets);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.btn_addPreset);
            this.splitContainer1.Panel2.Controls.Add(this.btn_removePreset);
            this.splitContainer1.Panel2.Controls.Add(this.btn_setDefault);
            this.splitContainer1.Size = new System.Drawing.Size(239, 526);
            this.splitContainer1.SplitterDistance = 485;
            this.splitContainer1.TabIndex = 1;
            // 
            // treeView_presets
            // 
            this.treeView_presets.ContextMenuStrip = this.presets_menu;
            this.treeView_presets.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeView_presets.ForeColor = System.Drawing.Color.DarkBlue;
            this.treeView_presets.FullRowSelect = true;
            this.treeView_presets.HideSelection = false;
            this.treeView_presets.ItemHeight = 21;
            this.treeView_presets.Location = new System.Drawing.Point(0, 0);
            this.treeView_presets.Name = "treeView_presets";
            this.treeView_presets.ShowLines = false;
            this.treeView_presets.Size = new System.Drawing.Size(239, 485);
            this.treeView_presets.TabIndex = 0;
            this.treeView_presets.MouseUp += new System.Windows.Forms.MouseEventHandler(this.treeview_presets_mouseUp);
            this.treeView_presets.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView_presets_AfterSelect);
            this.treeView_presets.KeyUp += new System.Windows.Forms.KeyEventHandler(this.treeView_presets_deleteKey);
            // 
            // presets_menu
            // 
            this.presets_menu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.pmnu_expandAll,
            this.pmnu_collapse,
            this.sep1,
            this.pmnu_delete});
            this.presets_menu.Name = "presets_menu";
            this.presets_menu.Size = new System.Drawing.Size(146, 76);
            // 
            // pmnu_expandAll
            // 
            this.pmnu_expandAll.Name = "pmnu_expandAll";
            this.pmnu_expandAll.Size = new System.Drawing.Size(145, 22);
            this.pmnu_expandAll.Text = "Expand All";
            this.pmnu_expandAll.Click += new System.EventHandler(this.pmnu_expandAll_Click);
            // 
            // pmnu_collapse
            // 
            this.pmnu_collapse.Name = "pmnu_collapse";
            this.pmnu_collapse.Size = new System.Drawing.Size(145, 22);
            this.pmnu_collapse.Text = "Collapse All";
            this.pmnu_collapse.Click += new System.EventHandler(this.pmnu_collapse_Click);
            // 
            // sep1
            // 
            this.sep1.Name = "sep1";
            this.sep1.Size = new System.Drawing.Size(142, 6);
            // 
            // pmnu_delete
            // 
            this.pmnu_delete.Name = "pmnu_delete";
            this.pmnu_delete.Size = new System.Drawing.Size(145, 22);
            this.pmnu_delete.Text = "Delete";
            this.pmnu_delete.Click += new System.EventHandler(this.pmnu_delete_click);
            // 
            // toolStrip1
            // 
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_source,
            this.toolStripSeparator10,
            this.btn_start,
            this.btn_add2Queue,
            this.btn_showQueue,
            this.toolStripSeparator4,
            this.btn_ActivityWindow,
            this.toolStripSeparator8,
            this.btn_minimize});
            this.toolStrip1.Location = new System.Drawing.Point(0, 24);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.toolStrip1.Size = new System.Drawing.Size(985, 39);
            this.toolStrip1.TabIndex = 1;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // btn_source
            // 
            this.btn_source.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_file_source,
            this.btn_dvd_source,
            this.toolStripSeparator1,
            this.mnu_dvd_drive});
            this.btn_source.Image = global::Handbrake.Properties.Resources.Movies;
            this.btn_source.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_source.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_source.Name = "btn_source";
            this.btn_source.Size = new System.Drawing.Size(90, 36);
            this.btn_source.Text = "Source";
            this.btn_source.ToolTipText = "Open a new source file or folder.";
            this.btn_source.Click += new System.EventHandler(this.btn_source_Click);
            // 
            // btn_file_source
            // 
            this.btn_file_source.Image = global::Handbrake.Properties.Resources.Movies_Small;
            this.btn_file_source.Name = "btn_file_source";
            this.btn_file_source.Size = new System.Drawing.Size(214, 22);
            this.btn_file_source.Text = "Video File";
            this.btn_file_source.Click += new System.EventHandler(this.btn_file_source_Click);
            // 
            // btn_dvd_source
            // 
            this.btn_dvd_source.Image = ((System.Drawing.Image)(resources.GetObject("btn_dvd_source.Image")));
            this.btn_dvd_source.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_dvd_source.Name = "btn_dvd_source";
            this.btn_dvd_source.Size = new System.Drawing.Size(214, 22);
            this.btn_dvd_source.Text = "DVD/ VIDEO_TS Folder";
            this.btn_dvd_source.Click += new System.EventHandler(this.btn_dvd_source_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(211, 6);
            // 
            // mnu_dvd_drive
            // 
            this.mnu_dvd_drive.Image = global::Handbrake.Properties.Resources.disc_small;
            this.mnu_dvd_drive.Name = "mnu_dvd_drive";
            this.mnu_dvd_drive.Size = new System.Drawing.Size(214, 22);
            this.mnu_dvd_drive.Text = "[No DVD Drive Ready]";
            this.mnu_dvd_drive.Visible = false;
            this.mnu_dvd_drive.Click += new System.EventHandler(this.mnu_dvd_drive_Click);
            // 
            // toolStripSeparator10
            // 
            this.toolStripSeparator10.Name = "toolStripSeparator10";
            this.toolStripSeparator10.Size = new System.Drawing.Size(6, 39);
            // 
            // btn_start
            // 
            this.btn_start.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_start.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_start.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_start.Name = "btn_start";
            this.btn_start.Size = new System.Drawing.Size(70, 36);
            this.btn_start.Text = "Start";
            this.btn_start.ToolTipText = "Start the encoding process";
            this.btn_start.Click += new System.EventHandler(this.btn_start_Click);
            // 
            // btn_add2Queue
            // 
            this.btn_add2Queue.Image = global::Handbrake.Properties.Resources.AddToQueue;
            this.btn_add2Queue.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_add2Queue.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_add2Queue.Name = "btn_add2Queue";
            this.btn_add2Queue.Size = new System.Drawing.Size(122, 36);
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
            this.btn_showQueue.Size = new System.Drawing.Size(115, 36);
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
            this.btn_ActivityWindow.Size = new System.Drawing.Size(132, 36);
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
            // btn_minimize
            // 
            this.btn_minimize.Image = global::Handbrake.Properties.Resources.hb32;
            this.btn_minimize.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_minimize.Name = "btn_minimize";
            this.btn_minimize.Size = new System.Drawing.Size(162, 36);
            this.btn_minimize.Text = "Minimize To System Tray";
            this.btn_minimize.Click += new System.EventHandler(this.btn_minimize_Click);
            // 
            // notifyIcon
            // 
            this.notifyIcon.BalloonTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.notifyIcon.BalloonTipText = "HandBrake Status Here";
            this.notifyIcon.BalloonTipTitle = "HandBrake";
            this.notifyIcon.ContextMenuStrip = notifyIconMenu;
            this.notifyIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon.Icon")));
            this.notifyIcon.Text = "HandBrake";
            this.notifyIcon.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.notifyIcon_MouseDoubleClick);
            // 
            // StatusStrip
            // 
            this.StatusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.lbl_encode});
            this.StatusStrip.Location = new System.Drawing.Point(0, 629);
            this.StatusStrip.Name = "StatusStrip";
            this.StatusStrip.Size = new System.Drawing.Size(985, 22);
            this.StatusStrip.TabIndex = 7;
            this.StatusStrip.Text = "statusStrip1";
            // 
            // lbl_encode
            // 
            this.lbl_encode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_encode.Name = "lbl_encode";
            this.lbl_encode.Size = new System.Drawing.Size(31, 17);
            this.lbl_encode.Text = "{0}";
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(985, 651);
            this.Controls.Add(this.GroupBox1);
            this.Controls.Add(this.groupBox_dest);
            this.Controls.Add(this.groupBox_output);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.advancedOptions);
            this.Controls.Add(this.frmMainMenu);
            this.Controls.Add(this.StatusStrip);
            this.DoubleBuffered = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(946, 668);
            this.Name = "frmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "HandBrake";
            notifyIconMenu.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.data_chpt)).EndInit();
            this.frmMainMenu.ResumeLayout(false);
            this.frmMainMenu.PerformLayout();
            this.GroupBox1.ResumeLayout(false);
            this.GroupBox1.PerformLayout();
            this.groupBox_output.ResumeLayout(false);
            this.groupBox_output.PerformLayout();
            this.TabPage2.ResumeLayout(false);
            this.TabPage2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar4)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar3)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
            this.TabPage3.ResumeLayout(false);
            this.TabPage3.PerformLayout();
            this.TabPage1.ResumeLayout(false);
            this.TabPage1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.slider_deblock)).EndInit();
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
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.presets_menu.ResumeLayout(false);
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.StatusStrip.ResumeLayout(false);
            this.StatusStrip.PerformLayout();
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
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator5;
        internal System.Windows.Forms.ToolStripMenuItem mnu_options;
        internal System.Windows.Forms.ToolStripMenuItem PresetsToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_presetReset;
        internal System.Windows.Forms.ToolStripMenuItem HelpToolStripMenuItem;
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
        internal System.Windows.Forms.TabPage TabPage2;
        internal System.Windows.Forms.ComboBox drp_audmix_1;
        internal System.Windows.Forms.ComboBox drp_track1Audio;
        internal System.Windows.Forms.ComboBox drp_audbit_1;
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
        internal System.Windows.Forms.Label Label1;
        internal System.Windows.Forms.Label Label53;
        internal System.Windows.Forms.Label Label52;
        internal System.Windows.Forms.Label Label51;
        internal System.Windows.Forms.Label Label15;
        internal System.Windows.Forms.TabControl advancedOptions;
        internal System.Windows.Forms.Label Label46;
        private System.Windows.Forms.GroupBox groupBox_dest;
        internal System.Windows.Forms.ComboBox drp_subtitle;
        internal System.Windows.Forms.Label Label19;
        internal System.Windows.Forms.Label Label20;
        internal System.Windows.Forms.CheckBox check_grayscale;
        internal System.Windows.Forms.Label label24;
        internal System.Windows.Forms.Label label11;
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
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator7;
        internal System.Windows.Forms.CheckBox check_iPodAtom;
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
        internal System.Windows.Forms.Button btn_clear;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton btn_start;
        private System.Windows.Forms.ToolStripButton btn_add2Queue;
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
        internal System.Windows.Forms.CheckBox check_Cabac;
        internal System.Windows.Forms.CheckBox check_noDCTDecimate;
        internal System.Windows.Forms.CheckBox check_noFastPSkip;
        internal System.Windows.Forms.Label lbl_trellis;
        internal System.Windows.Forms.ComboBox drop_trellis;
        internal System.Windows.Forms.ComboBox drop_deblockBeta;
        internal System.Windows.Forms.Label label41;
        internal System.Windows.Forms.ComboBox drop_deblockAlpha;
        internal System.Windows.Forms.Panel panel3;
        internal System.Windows.Forms.Panel panel1;
        internal System.Windows.Forms.Panel panel2;
        internal System.Windows.Forms.CheckBox check_8x8DCT;
        internal System.Windows.Forms.Label label45;
        internal System.Windows.Forms.ComboBox drop_analysis;
        internal System.Windows.Forms.Label label48;
        internal System.Windows.Forms.ComboBox drop_subpixelMotionEstimation;
        internal System.Windows.Forms.Label label49;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationRange;
        internal System.Windows.Forms.Label label54;
        internal System.Windows.Forms.ComboBox drop_MotionEstimationMethod;
        internal System.Windows.Forms.CheckBox check_pyrmidalBFrames;
        internal System.Windows.Forms.CheckBox check_weightedBFrames;
        internal System.Windows.Forms.Label lbl_direct_prediction;
        internal System.Windows.Forms.ComboBox drop_directPrediction;
        internal System.Windows.Forms.Label label62;
        internal System.Windows.Forms.ComboBox drop_bFrames;
        internal System.Windows.Forms.Label label64;
        internal System.Windows.Forms.ComboBox drop_refFrames;
        internal System.Windows.Forms.CheckBox check_mixedReferences;
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
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.Button btn_destBrowse;
        internal System.Windows.Forms.TrackBar trackBar1;
        internal System.Windows.Forms.Label lbl_drc4;
        internal System.Windows.Forms.Label lbl_drc3;
        internal System.Windows.Forms.Label lbl_drc2;
        internal System.Windows.Forms.TrackBar trackBar4;
        internal System.Windows.Forms.ComboBox drp_audsr_4;
        internal System.Windows.Forms.ComboBox drp_audbit_4;
        internal System.Windows.Forms.ComboBox drp_audenc_4;
        internal System.Windows.Forms.ComboBox drp_audmix_4;
        internal System.Windows.Forms.ComboBox drp_track4Audio;
        internal System.Windows.Forms.Label lbl_t4;
        internal System.Windows.Forms.TrackBar trackBar3;
        internal System.Windows.Forms.ComboBox drp_audsr_3;
        internal System.Windows.Forms.ComboBox drp_audbit_3;
        internal System.Windows.Forms.ComboBox drp_audenc_3;
        internal System.Windows.Forms.ComboBox drp_audmix_3;
        internal System.Windows.Forms.ComboBox drp_track3Audio;
        internal System.Windows.Forms.Label lbl_t3;
        internal System.Windows.Forms.TrackBar trackBar2;
        internal System.Windows.Forms.Label label16;
        internal System.Windows.Forms.Label lbl_drc1;
        internal System.Windows.Forms.TreeView treeView_presets;
        internal System.Windows.Forms.RichTextBox rtf_query;
        private System.Windows.Forms.NotifyIcon notifyIcon;
        private System.Windows.Forms.ToolStripButton btn_minimize;
        private System.Windows.Forms.ToolStripMenuItem btn_restore;
        internal System.Windows.Forms.Label lbl_src_res;
        internal System.Windows.Forms.Label label7;
        internal System.Windows.Forms.Label lbl_duration;
        internal System.Windows.Forms.Label label_duration;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator10;
        private System.Windows.Forms.ToolStripMenuItem btn_file_source;
        private System.Windows.Forms.ToolStripMenuItem mnu_delete_preset;
        private System.Windows.Forms.ToolStripMenuItem btn_new_preset;
        private System.Windows.Forms.ToolStripMenuItem mnu_handbrake_forums;
        private System.Windows.Forms.ToolStripMenuItem mnu_user_guide;
        private System.Windows.Forms.ToolStripDropDownButton btn_source;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem btn_dvd_source;
        internal System.Windows.Forms.ComboBox drop_format;
        internal System.Windows.Forms.Label label5;
        internal System.Windows.Forms.ToolStripMenuItem mnu_encodeLog;
        private System.Windows.Forms.StatusStrip StatusStrip;
        private System.Windows.Forms.ToolStripStatusLabel lbl_encode;
        internal System.Windows.Forms.CheckBox check_decomb;
        internal System.Windows.Forms.RadioButton check_customCrop;
        internal System.Windows.Forms.RadioButton check_autoCrop;
        internal System.Windows.Forms.Label lbl_deblockVal;
        internal System.Windows.Forms.TrackBar slider_deblock;
        internal System.Windows.Forms.ComboBox drp_deNoise;
        internal System.Windows.Forms.Label label8;
        internal System.Windows.Forms.OpenFileDialog ISO_Open;
        internal System.Windows.Forms.FolderBrowserDialog DVD_Open;
        internal System.Windows.Forms.ToolStripMenuItem mnu_dvd_drive;
        private System.Windows.Forms.ContextMenuStrip presets_menu;
        private System.Windows.Forms.ToolStripMenuItem pmnu_expandAll;
        private System.Windows.Forms.ToolStripMenuItem pmnu_collapse;
        private System.Windows.Forms.ToolStripSeparator sep1;
        private System.Windows.Forms.ToolStripMenuItem pmnu_delete;
        private System.Windows.Forms.SplitContainer splitContainer1;

    }
}