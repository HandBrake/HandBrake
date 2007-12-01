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
            this.DVD_Save = new System.Windows.Forms.SaveFileDialog();
            this.File_Save = new System.Windows.Forms.SaveFileDialog();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.drop_chapterFinish = new System.Windows.Forms.ComboBox();
            this.drop_chapterStart = new System.Windows.Forms.ComboBox();
            this.drp_dvdtitle = new System.Windows.Forms.ComboBox();
            this.RadioISO = new System.Windows.Forms.RadioButton();
            this.text_source = new System.Windows.Forms.TextBox();
            this.text_destination = new System.Windows.Forms.TextBox();
            this.btn_Browse = new System.Windows.Forms.Button();
            this.text_height = new System.Windows.Forms.TextBox();
            this.text_width = new System.Windows.Forms.TextBox();
            this.btn_destBrowse = new System.Windows.Forms.Button();
            this.drp_videoEncoder = new System.Windows.Forms.ComboBox();
            this.drp_audioCodec = new System.Windows.Forms.ComboBox();
            this.CheckCRF = new System.Windows.Forms.CheckBox();
            this.QueryEditorText = new System.Windows.Forms.RichTextBox();
            this.rtf_h264advanced = new System.Windows.Forms.RichTextBox();
            this.drp_audioBitrate = new System.Windows.Forms.ComboBox();
            this.drp_audioSampleRate = new System.Windows.Forms.ComboBox();
            this.check_largeFile = new System.Windows.Forms.CheckBox();
            this.check_turbo = new System.Windows.Forms.CheckBox();
            this.drp_videoFramerate = new System.Windows.Forms.ComboBox();
            this.slider_videoQuality = new System.Windows.Forms.TrackBar();
            this.text_filesize = new System.Windows.Forms.TextBox();
            this.text_bitrate = new System.Windows.Forms.TextBox();
            this.drp_subtitle = new System.Windows.Forms.ComboBox();
            this.btn_setDefault = new System.Windows.Forms.Button();
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
            this.mnu_showPresets = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_SelectDefault = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator7 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_presetReset = new System.Windows.Forms.ToolStripMenuItem();
            this.HelpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_quickStart = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.OnlineDocumentationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_wiki = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_faq = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_onlineDocs = new System.Windows.Forms.ToolStripMenuItem();
            this.WebsiteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_homepage = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_forum = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_UpdateCheck = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_about = new System.Windows.Forms.ToolStripMenuItem();
            this.frmMainMenu = new System.Windows.Forms.MenuStrip();
            this.GroupBox1 = new System.Windows.Forms.GroupBox();
            this.Label13 = new System.Windows.Forms.Label();
            this.RadioDVD = new System.Windows.Forms.RadioButton();
            this.Label17 = new System.Windows.Forms.Label();
            this.Label9 = new System.Windows.Forms.Label();
            this.Label10 = new System.Windows.Forms.Label();
            this.groupBox_output = new System.Windows.Forms.GroupBox();
            this.Label56 = new System.Windows.Forms.Label();
            this.lbl_Aspect = new System.Windows.Forms.Label();
            this.Label91 = new System.Windows.Forms.Label();
            this.Label55 = new System.Windows.Forms.Label();
            this.Label47 = new System.Windows.Forms.Label();
            this.Label12 = new System.Windows.Forms.Label();
            this.Label3 = new System.Windows.Forms.Label();
            this.lbl_update = new System.Windows.Forms.Label();
            this.btn_queue = new System.Windows.Forms.Button();
            this.btn_encode = new System.Windows.Forms.Button();
            this.Version = new System.Windows.Forms.Label();
            this.lbl_encode = new System.Windows.Forms.Label();
            this.btn_eCancel = new System.Windows.Forms.Button();
            this.TabPage6 = new System.Windows.Forms.TabPage();
            this.btn_copy = new System.Windows.Forms.Button();
            this.label23 = new System.Windows.Forms.Label();
            this.Label7 = new System.Windows.Forms.Label();
            this.Label39 = new System.Windows.Forms.Label();
            this.btn_ClearQuery = new System.Windows.Forms.Button();
            this.GenerateQuery = new System.Windows.Forms.Button();
            this.h264Tab = new System.Windows.Forms.TabPage();
            this.Label43 = new System.Windows.Forms.Label();
            this.label_h264 = new System.Windows.Forms.LinkLabel();
            this.Label95 = new System.Windows.Forms.Label();
            this.btn_h264Clear = new System.Windows.Forms.Button();
            this.Label90 = new System.Windows.Forms.Label();
            this.Label92 = new System.Windows.Forms.Label();
            this.TabPage2 = new System.Windows.Forms.TabPage();
            this.Label19 = new System.Windows.Forms.Label();
            this.Label21 = new System.Windows.Forms.Label();
            this.Label20 = new System.Windows.Forms.Label();
            this.Label29 = new System.Windows.Forms.Label();
            this.drp_audioMixDown = new System.Windows.Forms.ComboBox();
            this.drp_audioChannels = new System.Windows.Forms.ComboBox();
            this.Label14 = new System.Windows.Forms.Label();
            this.Label5 = new System.Windows.Forms.Label();
            this.Label16 = new System.Windows.Forms.Label();
            this.Label32 = new System.Windows.Forms.Label();
            this.Label18 = new System.Windows.Forms.Label();
            this.TabPage3 = new System.Windows.Forms.TabPage();
            this.check_iPodAtom = new System.Windows.Forms.CheckBox();
            this.check_optimiseMP4 = new System.Windows.Forms.CheckBox();
            this.check_grayscale = new System.Windows.Forms.CheckBox();
            this.lbl_largeMp4Warning = new System.Windows.Forms.Label();
            this.Label22 = new System.Windows.Forms.Label();
            this.check_2PassEncode = new System.Windows.Forms.CheckBox();
            this.Label2 = new System.Windows.Forms.Label();
            this.Label42 = new System.Windows.Forms.Label();
            this.SliderValue = new System.Windows.Forms.Label();
            this.Label46 = new System.Windows.Forms.Label();
            this.Label40 = new System.Windows.Forms.Label();
            this.TabPage1 = new System.Windows.Forms.TabPage();
            this.check_lAnamorphic = new System.Windows.Forms.CheckBox();
            this.check_vfr = new System.Windows.Forms.CheckBox();
            this.label24 = new System.Windows.Forms.Label();
            this.drp_deNoise = new System.Windows.Forms.ComboBox();
            this.label11 = new System.Windows.Forms.Label();
            this.check_deblock = new System.Windows.Forms.CheckBox();
            this.check_detelecine = new System.Windows.Forms.CheckBox();
            this.label4 = new System.Windows.Forms.Label();
            this.drp_deInterlace_option = new System.Windows.Forms.ComboBox();
            this.Check_ChapterMarkers = new System.Windows.Forms.CheckBox();
            this.CheckPixelRatio = new System.Windows.Forms.CheckBox();
            this.label6 = new System.Windows.Forms.Label();
            this.lbl_RecomendedCrop = new System.Windows.Forms.Label();
            this.Label8 = new System.Windows.Forms.Label();
            this.Label1 = new System.Windows.Forms.Label();
            this.Label53 = new System.Windows.Forms.Label();
            this.Label52 = new System.Windows.Forms.Label();
            this.Label51 = new System.Windows.Forms.Label();
            this.Label50 = new System.Windows.Forms.Label();
            this.Label15 = new System.Windows.Forms.Label();
            this.text_top = new System.Windows.Forms.TextBox();
            this.text_bottom = new System.Windows.Forms.TextBox();
            this.drp_crop = new System.Windows.Forms.ComboBox();
            this.text_right = new System.Windows.Forms.TextBox();
            this.text_left = new System.Windows.Forms.TextBox();
            this.advancedOptions = new System.Windows.Forms.TabControl();
            this.groupBox_dest = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.treeView_presets = new System.Windows.Forms.TreeView();
            this.lbl_vfr = new System.Windows.Forms.Label();
            Label38 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).BeginInit();
            this.frmMainMenu.SuspendLayout();
            this.GroupBox1.SuspendLayout();
            this.groupBox_output.SuspendLayout();
            this.TabPage6.SuspendLayout();
            this.h264Tab.SuspendLayout();
            this.TabPage2.SuspendLayout();
            this.TabPage3.SuspendLayout();
            this.TabPage1.SuspendLayout();
            this.advancedOptions.SuspendLayout();
            this.groupBox_dest.SuspendLayout();
            this.groupBox2.SuspendLayout();
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
            // 
            // drop_chapterFinish
            // 
            this.drop_chapterFinish.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_chapterFinish.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_chapterFinish.FormattingEnabled = true;
            this.drop_chapterFinish.Location = new System.Drawing.Point(408, 55);
            this.drop_chapterFinish.Name = "drop_chapterFinish";
            this.drop_chapterFinish.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterFinish.TabIndex = 10;
            this.drop_chapterFinish.Text = "Auto";
            this.ToolTip.SetToolTip(this.drop_chapterFinish, "Step 3 - Select the chapter range you would like to enocde. (default: All Chapter" +
                    "s)");
            this.drop_chapterFinish.SelectedIndexChanged += new System.EventHandler(this.drop_chapterFinish_SelectedIndexChanged);
            // 
            // drop_chapterStart
            // 
            this.drop_chapterStart.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_chapterStart.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_chapterStart.FormattingEnabled = true;
            this.drop_chapterStart.Location = new System.Drawing.Point(295, 55);
            this.drop_chapterStart.Name = "drop_chapterStart";
            this.drop_chapterStart.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterStart.TabIndex = 9;
            this.drop_chapterStart.Text = "Auto";
            this.ToolTip.SetToolTip(this.drop_chapterStart, "Step 3 - Select the chapter range you would like to enocde. (default: All Chapter" +
                    "s)");
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
            this.ToolTip.SetToolTip(this.drp_dvdtitle, "Step 2 - Select the title number you wish to encode.");
            this.drp_dvdtitle.SelectedIndexChanged += new System.EventHandler(this.drp_dvdtitle_SelectedIndexChanged);
            this.drp_dvdtitle.Click += new System.EventHandler(this.drp_dvdtitle_Click);
            // 
            // RadioISO
            // 
            this.RadioISO.AutoSize = true;
            this.RadioISO.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.RadioISO.Location = new System.Drawing.Point(549, 22);
            this.RadioISO.Name = "RadioISO";
            this.RadioISO.Size = new System.Drawing.Size(44, 17);
            this.RadioISO.TabIndex = 4;
            this.RadioISO.Text = "File";
            this.ToolTip.SetToolTip(this.RadioISO, "ISO, TS, MPG");
            this.RadioISO.UseVisualStyleBackColor = true;
            // 
            // text_source
            // 
            this.text_source.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_source.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_source.Location = new System.Drawing.Point(99, 19);
            this.text_source.Name = "text_source";
            this.text_source.Size = new System.Drawing.Size(294, 21);
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
            this.text_destination.Size = new System.Drawing.Size(294, 21);
            this.text_destination.TabIndex = 1;
            this.ToolTip.SetToolTip(this.text_destination, "Location where the enoceded file will be saved.");
            // 
            // btn_Browse
            // 
            this.btn_Browse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_Browse.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_Browse.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_Browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Browse.Location = new System.Drawing.Point(399, 19);
            this.btn_Browse.Name = "btn_Browse";
            this.btn_Browse.Size = new System.Drawing.Size(78, 22);
            this.btn_Browse.TabIndex = 5;
            this.btn_Browse.Text = "Browse";
            this.ToolTip.SetToolTip(this.btn_Browse, "Step 1 - Select a Source. This can be either a DVD or ts/mpg/iso file");
            this.btn_Browse.UseVisualStyleBackColor = true;
            this.btn_Browse.Click += new System.EventHandler(this.btn_Browse_Click);
            // 
            // text_height
            // 
            this.text_height.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_height.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_height.ForeColor = System.Drawing.SystemColors.InfoText;
            this.text_height.Location = new System.Drawing.Point(196, 54);
            this.text_height.Name = "text_height";
            this.text_height.Size = new System.Drawing.Size(64, 21);
            this.text_height.TabIndex = 7;
            this.ToolTip.SetToolTip(this.text_height, "Video Resolution (Height)");
            this.text_height.TextChanged += new System.EventHandler(this.text_height_TextChanged);
            // 
            // text_width
            // 
            this.text_width.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_width.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_width.Location = new System.Drawing.Point(104, 54);
            this.text_width.Name = "text_width";
            this.text_width.Size = new System.Drawing.Size(64, 21);
            this.text_width.TabIndex = 5;
            this.ToolTip.SetToolTip(this.text_width, "Video Resolution (Width)");
            this.text_width.TextChanged += new System.EventHandler(this.text_width_TextChanged);
            // 
            // btn_destBrowse
            // 
            this.btn_destBrowse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_destBrowse.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_destBrowse.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_destBrowse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_destBrowse.Location = new System.Drawing.Point(399, 18);
            this.btn_destBrowse.Name = "btn_destBrowse";
            this.btn_destBrowse.Size = new System.Drawing.Size(83, 22);
            this.btn_destBrowse.TabIndex = 2;
            this.btn_destBrowse.Text = "Browse";
            this.ToolTip.SetToolTip(this.btn_destBrowse, "Step 4 - Select a location to save your encoded file.");
            this.btn_destBrowse.UseVisualStyleBackColor = true;
            this.btn_destBrowse.Click += new System.EventHandler(this.btn_destBrowse_Click);
            // 
            // drp_videoEncoder
            // 
            this.drp_videoEncoder.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_videoEncoder.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_videoEncoder.FormattingEnabled = true;
            this.drp_videoEncoder.Items.AddRange(new object[] {
            "Mpeg 4",
            "Xvid",
            "H.264",
            "H.264 (iPod)"});
            this.drp_videoEncoder.Location = new System.Drawing.Point(99, 20);
            this.drp_videoEncoder.Name = "drp_videoEncoder";
            this.drp_videoEncoder.Size = new System.Drawing.Size(156, 21);
            this.drp_videoEncoder.TabIndex = 1;
            this.drp_videoEncoder.Text = "H.264";
            this.ToolTip.SetToolTip(this.drp_videoEncoder, "Step 5 - Select a video encoder");
            this.drp_videoEncoder.SelectedIndexChanged += new System.EventHandler(this.drp_videoEncoder_SelectedIndexChanged);
            // 
            // drp_audioCodec
            // 
            this.drp_audioCodec.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audioCodec.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audioCodec.FormattingEnabled = true;
            this.drp_audioCodec.Items.AddRange(new object[] {
            "AAC",
            "MP3",
            "Vorbis",
            "AC3"});
            this.drp_audioCodec.Location = new System.Drawing.Point(376, 20);
            this.drp_audioCodec.Name = "drp_audioCodec";
            this.drp_audioCodec.Size = new System.Drawing.Size(111, 21);
            this.drp_audioCodec.TabIndex = 3;
            this.drp_audioCodec.Text = "AAC";
            this.ToolTip.SetToolTip(this.drp_audioCodec, "Step 6 - Select an audio encoder.");
            this.drp_audioCodec.SelectedIndexChanged += new System.EventHandler(this.drp_audioCodec_SelectedIndexChanged);
            // 
            // CheckCRF
            // 
            this.CheckCRF.AutoSize = true;
            this.CheckCRF.BackColor = System.Drawing.Color.Transparent;
            this.CheckCRF.Enabled = false;
            this.CheckCRF.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.CheckCRF.Location = new System.Drawing.Point(446, 133);
            this.CheckCRF.Name = "CheckCRF";
            this.CheckCRF.Size = new System.Drawing.Size(146, 17);
            this.CheckCRF.TabIndex = 16;
            this.CheckCRF.Text = "Constant Rate Factor";
            this.ToolTip.SetToolTip(this.CheckCRF, "Constant Rate Factor");
            this.CheckCRF.UseVisualStyleBackColor = false;
            // 
            // QueryEditorText
            // 
            this.QueryEditorText.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.QueryEditorText.Location = new System.Drawing.Point(16, 88);
            this.QueryEditorText.Name = "QueryEditorText";
            this.QueryEditorText.Size = new System.Drawing.Size(605, 93);
            this.QueryEditorText.TabIndex = 3;
            this.QueryEditorText.Text = "";
            this.ToolTip.SetToolTip(this.QueryEditorText, "Manually add or edit a query generated by the GUI.");
            // 
            // rtf_h264advanced
            // 
            this.rtf_h264advanced.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.rtf_h264advanced.Location = new System.Drawing.Point(16, 79);
            this.rtf_h264advanced.Name = "rtf_h264advanced";
            this.rtf_h264advanced.Size = new System.Drawing.Size(605, 76);
            this.rtf_h264advanced.TabIndex = 2;
            this.rtf_h264advanced.Text = "";
            this.ToolTip.SetToolTip(this.rtf_h264advanced, "H.264 advanced options can be added here. See link below for details.");
            // 
            // drp_audioBitrate
            // 
            this.drp_audioBitrate.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audioBitrate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audioBitrate.FormattingEnabled = true;
            this.drp_audioBitrate.Items.AddRange(new object[] {
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
            this.drp_audioBitrate.Location = new System.Drawing.Point(137, 37);
            this.drp_audioBitrate.Name = "drp_audioBitrate";
            this.drp_audioBitrate.Size = new System.Drawing.Size(101, 21);
            this.drp_audioBitrate.TabIndex = 5;
            this.drp_audioBitrate.Text = "128";
            this.ToolTip.SetToolTip(this.drp_audioBitrate, "Set the Audio Bit-Rate");
            // 
            // drp_audioSampleRate
            // 
            this.drp_audioSampleRate.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audioSampleRate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audioSampleRate.FormattingEnabled = true;
            this.drp_audioSampleRate.Items.AddRange(new object[] {
            "48",
            "44.1",
            "32",
            "24",
            "22.05"});
            this.drp_audioSampleRate.Location = new System.Drawing.Point(137, 67);
            this.drp_audioSampleRate.Name = "drp_audioSampleRate";
            this.drp_audioSampleRate.Size = new System.Drawing.Size(101, 21);
            this.drp_audioSampleRate.TabIndex = 6;
            this.drp_audioSampleRate.Text = "44.1";
            this.ToolTip.SetToolTip(this.drp_audioSampleRate, "Set the Audio Sample Rate");
            // 
            // check_largeFile
            // 
            this.check_largeFile.AutoSize = true;
            this.check_largeFile.BackColor = System.Drawing.Color.Transparent;
            this.check_largeFile.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_largeFile.Location = new System.Drawing.Point(16, 107);
            this.check_largeFile.Name = "check_largeFile";
            this.check_largeFile.Size = new System.Drawing.Size(172, 17);
            this.check_largeFile.TabIndex = 4;
            this.check_largeFile.Text = "Larger mp4 Files (> 4GB)";
            this.ToolTip.SetToolTip(this.check_largeFile, "Allows creation of mp4 files greater than 4GB.");
            this.check_largeFile.UseVisualStyleBackColor = false;
            this.check_largeFile.CheckedChanged += new System.EventHandler(this.check_largeFile_CheckedChanged);
            // 
            // check_turbo
            // 
            this.check_turbo.AutoSize = true;
            this.check_turbo.BackColor = System.Drawing.Color.Transparent;
            this.check_turbo.Enabled = false;
            this.check_turbo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_turbo.Location = new System.Drawing.Point(37, 84);
            this.check_turbo.Name = "check_turbo";
            this.check_turbo.Size = new System.Drawing.Size(110, 17);
            this.check_turbo.TabIndex = 3;
            this.check_turbo.Text = "Turbo 1st Pass";
            this.ToolTip.SetToolTip(this.check_turbo, "Makes the first pass of a 2 pass encode faster.");
            this.check_turbo.UseVisualStyleBackColor = false;
            // 
            // drp_videoFramerate
            // 
            this.drp_videoFramerate.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_videoFramerate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_videoFramerate.FormattingEnabled = true;
            this.drp_videoFramerate.Items.AddRange(new object[] {
            "Automatic",
            "5",
            "10",
            "12",
            "15",
            "23.976",
            "24",
            "25",
            "29.97"});
            this.drp_videoFramerate.Location = new System.Drawing.Point(133, 189);
            this.drp_videoFramerate.Name = "drp_videoFramerate";
            this.drp_videoFramerate.Size = new System.Drawing.Size(81, 21);
            this.drp_videoFramerate.TabIndex = 7;
            this.drp_videoFramerate.Text = "Automatic";
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
            this.drp_subtitle.Location = new System.Drawing.Point(406, 36);
            this.drp_subtitle.Name = "drp_subtitle";
            this.drp_subtitle.Size = new System.Drawing.Size(213, 21);
            this.drp_subtitle.TabIndex = 12;
            this.drp_subtitle.Text = "None";
            this.ToolTip.SetToolTip(this.drp_subtitle, "Select the subtitle language you require from this dropdown.");
            // 
            // btn_setDefault
            // 
            this.btn_setDefault.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_setDefault.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_setDefault.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_setDefault.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_setDefault.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_setDefault.Location = new System.Drawing.Point(113, 446);
            this.btn_setDefault.Name = "btn_setDefault";
            this.btn_setDefault.Size = new System.Drawing.Size(72, 22);
            this.btn_setDefault.TabIndex = 1;
            this.btn_setDefault.TabStop = false;
            this.btn_setDefault.Text = "Set Default";
            this.ToolTip.SetToolTip(this.btn_setDefault, "Set current settings as program defaults.\r\nRequires option to be enabled in Tools" +
                    " > Options");
            this.btn_setDefault.UseVisualStyleBackColor = false;
            this.btn_setDefault.Click += new System.EventHandler(this.btn_setDefault_Click);
            // 
            // DVD_Open
            // 
            this.DVD_Open.Description = "Select the \"VIDEO_TS\" folder from your DVD Drvie.";
            // 
            // File_Open
            // 
            this.File_Open.DefaultExt = "hb";
            this.File_Open.Filter = "hb|*.hb";
            // 
            // ISO_Open
            // 
            this.ISO_Open.DefaultExt = "ISO";
            this.ISO_Open.Filter = "All Supported Files|*.iso;*.mpg;*.m2t;*.vob;*.ts;*.mpeg;";
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
            this.FileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
            this.FileToolStripMenuItem.Text = "&File";
            // 
            // mnu_open
            // 
            this.mnu_open.Image = ((System.Drawing.Image)(resources.GetObject("mnu_open.Image")));
            this.mnu_open.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.mnu_open.Name = "mnu_open";
            this.mnu_open.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.mnu_open.Size = new System.Drawing.Size(185, 22);
            this.mnu_open.Text = "&Open Preset";
            this.mnu_open.Click += new System.EventHandler(this.mnu_open_Click);
            // 
            // mnu_save
            // 
            this.mnu_save.Image = ((System.Drawing.Image)(resources.GetObject("mnu_save.Image")));
            this.mnu_save.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.mnu_save.Name = "mnu_save";
            this.mnu_save.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            this.mnu_save.Size = new System.Drawing.Size(185, 22);
            this.mnu_save.Text = "&Save Preset";
            this.mnu_save.Click += new System.EventHandler(this.mnu_save_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(182, 6);
            // 
            // mnu_exit
            // 
            this.mnu_exit.Name = "mnu_exit";
            this.mnu_exit.Size = new System.Drawing.Size(185, 22);
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
            this.ToolsToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.ToolsToolStripMenuItem.Text = "&Tools";
            // 
            // mnu_encode
            // 
            this.mnu_encode.Image = global::Handbrake.Properties.Resources.Queue_Small;
            this.mnu_encode.Name = "mnu_encode";
            this.mnu_encode.Size = new System.Drawing.Size(155, 22);
            this.mnu_encode.Text = "View Queue";
            this.mnu_encode.Click += new System.EventHandler(this.mnu_encode_Click);
            // 
            // mnu_viewDVDdata
            // 
            this.mnu_viewDVDdata.Image = global::Handbrake.Properties.Resources.Output_Small;
            this.mnu_viewDVDdata.Name = "mnu_viewDVDdata";
            this.mnu_viewDVDdata.Size = new System.Drawing.Size(155, 22);
            this.mnu_viewDVDdata.Text = "View DVD data";
            this.mnu_viewDVDdata.Click += new System.EventHandler(this.mnu_viewDVDdata_Click);
            // 
            // ToolStripSeparator5
            // 
            this.ToolStripSeparator5.Name = "ToolStripSeparator5";
            this.ToolStripSeparator5.Size = new System.Drawing.Size(152, 6);
            // 
            // mnu_options
            // 
            this.mnu_options.Image = global::Handbrake.Properties.Resources.Pref_Small;
            this.mnu_options.Name = "mnu_options";
            this.mnu_options.Size = new System.Drawing.Size(155, 22);
            this.mnu_options.Text = "Options";
            this.mnu_options.Click += new System.EventHandler(this.mnu_options_Click);
            // 
            // PresetsToolStripMenuItem
            // 
            this.PresetsToolStripMenuItem.BackColor = System.Drawing.SystemColors.ControlLight;
            this.PresetsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_showPresets,
            this.mnu_SelectDefault,
            this.toolStripSeparator7,
            this.mnu_presetReset});
            this.PresetsToolStripMenuItem.Name = "PresetsToolStripMenuItem";
            this.PresetsToolStripMenuItem.Size = new System.Drawing.Size(55, 20);
            this.PresetsToolStripMenuItem.Text = "&Presets";
            // 
            // mnu_showPresets
            // 
            this.mnu_showPresets.Name = "mnu_showPresets";
            this.mnu_showPresets.Size = new System.Drawing.Size(194, 22);
            this.mnu_showPresets.Text = "Show Presets";
            this.mnu_showPresets.Visible = false;
            this.mnu_showPresets.Click += new System.EventHandler(this.mnu_showPresets_Click);
            // 
            // mnu_SelectDefault
            // 
            this.mnu_SelectDefault.Name = "mnu_SelectDefault";
            this.mnu_SelectDefault.Size = new System.Drawing.Size(194, 22);
            this.mnu_SelectDefault.Text = "Select Default Preset";
            this.mnu_SelectDefault.ToolTipText = "Select HandBrake\'s default preset";
            this.mnu_SelectDefault.Click += new System.EventHandler(this.mnu_SelectDefault_Click);
            // 
            // toolStripSeparator7
            // 
            this.toolStripSeparator7.Name = "toolStripSeparator7";
            this.toolStripSeparator7.Size = new System.Drawing.Size(191, 6);
            // 
            // mnu_presetReset
            // 
            this.mnu_presetReset.Name = "mnu_presetReset";
            this.mnu_presetReset.Size = new System.Drawing.Size(194, 22);
            this.mnu_presetReset.Text = "Update Built-in Presets";
            this.mnu_presetReset.ToolTipText = "Resets all presets.";
            this.mnu_presetReset.Click += new System.EventHandler(this.mnu_presetReset_Click);
            // 
            // HelpToolStripMenuItem
            // 
            this.HelpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_quickStart,
            this.toolStripSeparator1,
            this.OnlineDocumentationToolStripMenuItem,
            this.WebsiteToolStripMenuItem,
            this.ToolStripSeparator3,
            this.mnu_UpdateCheck,
            this.toolStripSeparator6,
            this.mnu_about});
            this.HelpToolStripMenuItem.Name = "HelpToolStripMenuItem";
            this.HelpToolStripMenuItem.Size = new System.Drawing.Size(40, 20);
            this.HelpToolStripMenuItem.Text = "&Help";
            // 
            // mnu_quickStart
            // 
            this.mnu_quickStart.Name = "mnu_quickStart";
            this.mnu_quickStart.Size = new System.Drawing.Size(197, 22);
            this.mnu_quickStart.Text = "Quick Start Information";
            this.mnu_quickStart.Click += new System.EventHandler(this.mnu_quickStart_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(194, 6);
            // 
            // OnlineDocumentationToolStripMenuItem
            // 
            this.OnlineDocumentationToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_wiki,
            this.mnu_faq,
            this.mnu_onlineDocs});
            this.OnlineDocumentationToolStripMenuItem.Name = "OnlineDocumentationToolStripMenuItem";
            this.OnlineDocumentationToolStripMenuItem.Size = new System.Drawing.Size(197, 22);
            this.OnlineDocumentationToolStripMenuItem.Text = "Online Documentation";
            // 
            // mnu_wiki
            // 
            this.mnu_wiki.Name = "mnu_wiki";
            this.mnu_wiki.Size = new System.Drawing.Size(195, 22);
            this.mnu_wiki.Text = "Wiki / User Guides";
            this.mnu_wiki.Click += new System.EventHandler(this.mnu_wiki_Click);
            // 
            // mnu_faq
            // 
            this.mnu_faq.Name = "mnu_faq";
            this.mnu_faq.Size = new System.Drawing.Size(195, 22);
            this.mnu_faq.Text = "FAQ";
            this.mnu_faq.Click += new System.EventHandler(this.mnu_faq_Click);
            // 
            // mnu_onlineDocs
            // 
            this.mnu_onlineDocs.Name = "mnu_onlineDocs";
            this.mnu_onlineDocs.Size = new System.Drawing.Size(195, 22);
            this.mnu_onlineDocs.Text = "Full Documentation List";
            this.mnu_onlineDocs.Click += new System.EventHandler(this.mnu_onlineDocs_Click);
            // 
            // WebsiteToolStripMenuItem
            // 
            this.WebsiteToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_homepage,
            this.mnu_forum});
            this.WebsiteToolStripMenuItem.Name = "WebsiteToolStripMenuItem";
            this.WebsiteToolStripMenuItem.Size = new System.Drawing.Size(197, 22);
            this.WebsiteToolStripMenuItem.Text = "Website";
            // 
            // mnu_homepage
            // 
            this.mnu_homepage.Name = "mnu_homepage";
            this.mnu_homepage.Size = new System.Drawing.Size(136, 22);
            this.mnu_homepage.Text = "Homepage";
            this.mnu_homepage.Click += new System.EventHandler(this.mnu_homepage_Click);
            // 
            // mnu_forum
            // 
            this.mnu_forum.Name = "mnu_forum";
            this.mnu_forum.Size = new System.Drawing.Size(136, 22);
            this.mnu_forum.Text = "Forum";
            this.mnu_forum.Click += new System.EventHandler(this.mnu_forum_Click);
            // 
            // ToolStripSeparator3
            // 
            this.ToolStripSeparator3.Name = "ToolStripSeparator3";
            this.ToolStripSeparator3.Size = new System.Drawing.Size(194, 6);
            // 
            // mnu_UpdateCheck
            // 
            this.mnu_UpdateCheck.Name = "mnu_UpdateCheck";
            this.mnu_UpdateCheck.Size = new System.Drawing.Size(197, 22);
            this.mnu_UpdateCheck.Text = "Check for Updates";
            this.mnu_UpdateCheck.Click += new System.EventHandler(this.mnu_UpdateCheck_Click);
            // 
            // toolStripSeparator6
            // 
            this.toolStripSeparator6.Name = "toolStripSeparator6";
            this.toolStripSeparator6.Size = new System.Drawing.Size(194, 6);
            // 
            // mnu_about
            // 
            this.mnu_about.Name = "mnu_about";
            this.mnu_about.Size = new System.Drawing.Size(197, 22);
            this.mnu_about.Text = "About...";
            this.mnu_about.Click += new System.EventHandler(this.mnu_about_Click);
            // 
            // frmMainMenu
            // 
            this.frmMainMenu.BackColor = System.Drawing.SystemColors.ControlLight;
            this.frmMainMenu.GripStyle = System.Windows.Forms.ToolStripGripStyle.Visible;
            this.frmMainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileToolStripMenuItem,
            this.ToolsToolStripMenuItem,
            this.PresetsToolStripMenuItem,
            this.HelpToolStripMenuItem});
            this.frmMainMenu.Location = new System.Drawing.Point(0, 0);
            this.frmMainMenu.Name = "frmMainMenu";
            this.frmMainMenu.Size = new System.Drawing.Size(874, 24);
            this.frmMainMenu.TabIndex = 0;
            this.frmMainMenu.Text = "MenuStrip1";
            // 
            // GroupBox1
            // 
            this.GroupBox1.BackColor = System.Drawing.SystemColors.ControlLight;
            this.GroupBox1.Controls.Add(this.Label13);
            this.GroupBox1.Controls.Add(this.drop_chapterFinish);
            this.GroupBox1.Controls.Add(this.drop_chapterStart);
            this.GroupBox1.Controls.Add(this.drp_dvdtitle);
            this.GroupBox1.Controls.Add(this.RadioDVD);
            this.GroupBox1.Controls.Add(this.RadioISO);
            this.GroupBox1.Controls.Add(this.btn_Browse);
            this.GroupBox1.Controls.Add(this.Label17);
            this.GroupBox1.Controls.Add(this.text_source);
            this.GroupBox1.Controls.Add(this.Label9);
            this.GroupBox1.Controls.Add(this.Label10);
            this.GroupBox1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GroupBox1.ForeColor = System.Drawing.Color.Black;
            this.GroupBox1.Location = new System.Drawing.Point(14, 35);
            this.GroupBox1.Name = "GroupBox1";
            this.GroupBox1.Size = new System.Drawing.Size(647, 87);
            this.GroupBox1.TabIndex = 1;
            this.GroupBox1.TabStop = false;
            this.GroupBox1.Text = "Source";
            // 
            // Label13
            // 
            this.Label13.AutoSize = true;
            this.Label13.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label13.Location = new System.Drawing.Point(374, 58);
            this.Label13.Name = "Label13";
            this.Label13.Size = new System.Drawing.Size(21, 13);
            this.Label13.TabIndex = 10;
            this.Label13.Text = "To";
            // 
            // RadioDVD
            // 
            this.RadioDVD.AutoSize = true;
            this.RadioDVD.Checked = true;
            this.RadioDVD.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.RadioDVD.Location = new System.Drawing.Point(483, 22);
            this.RadioDVD.Name = "RadioDVD";
            this.RadioDVD.Size = new System.Drawing.Size(60, 17);
            this.RadioDVD.TabIndex = 3;
            this.RadioDVD.TabStop = true;
            this.RadioDVD.Text = "Folder";
            this.RadioDVD.UseVisualStyleBackColor = true;
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
            this.Label9.Location = new System.Drawing.Point(224, 58);
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
            this.groupBox_output.BackColor = System.Drawing.SystemColors.ControlLight;
            this.groupBox_output.Controls.Add(this.Label56);
            this.groupBox_output.Controls.Add(this.lbl_Aspect);
            this.groupBox_output.Controls.Add(this.Label91);
            this.groupBox_output.Controls.Add(this.text_height);
            this.groupBox_output.Controls.Add(this.Label55);
            this.groupBox_output.Controls.Add(this.text_width);
            this.groupBox_output.Controls.Add(this.drp_videoEncoder);
            this.groupBox_output.Controls.Add(this.Label47);
            this.groupBox_output.Controls.Add(this.drp_audioCodec);
            this.groupBox_output.Controls.Add(this.Label12);
            this.groupBox_output.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox_output.ForeColor = System.Drawing.Color.Black;
            this.groupBox_output.Location = new System.Drawing.Point(14, 185);
            this.groupBox_output.Name = "groupBox_output";
            this.groupBox_output.Size = new System.Drawing.Size(647, 85);
            this.groupBox_output.TabIndex = 3;
            this.groupBox_output.TabStop = false;
            this.groupBox_output.Text = "Output Settings (Preset: None)";
            // 
            // Label56
            // 
            this.Label56.AutoSize = true;
            this.Label56.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label56.ForeColor = System.Drawing.Color.Black;
            this.Label56.Location = new System.Drawing.Point(175, 57);
            this.Label56.Name = "Label56";
            this.Label56.Size = new System.Drawing.Size(15, 13);
            this.Label56.TabIndex = 6;
            this.Label56.Text = "x";
            // 
            // lbl_Aspect
            // 
            this.lbl_Aspect.AutoSize = true;
            this.lbl_Aspect.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_Aspect.Location = new System.Drawing.Point(374, 57);
            this.lbl_Aspect.Name = "lbl_Aspect";
            this.lbl_Aspect.Size = new System.Drawing.Size(72, 12);
            this.lbl_Aspect.TabIndex = 9;
            this.lbl_Aspect.Text = "Select a Title";
            // 
            // Label91
            // 
            this.Label91.AutoSize = true;
            this.Label91.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label91.Location = new System.Drawing.Point(270, 57);
            this.Label91.Name = "Label91";
            this.Label91.Size = new System.Drawing.Size(87, 13);
            this.Label91.TabIndex = 8;
            this.Label91.Text = "Aspect Ratio: ";
            // 
            // Label55
            // 
            this.Label55.AutoSize = true;
            this.Label55.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label55.ForeColor = System.Drawing.Color.Black;
            this.Label55.Location = new System.Drawing.Point(17, 57);
            this.Label55.Name = "Label55";
            this.Label55.Size = new System.Drawing.Size(85, 13);
            this.Label55.TabIndex = 4;
            this.Label55.Text = "Width/Height:";
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
            // Label12
            // 
            this.Label12.AutoSize = true;
            this.Label12.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label12.ForeColor = System.Drawing.Color.Black;
            this.Label12.Location = new System.Drawing.Point(270, 24);
            this.Label12.Name = "Label12";
            this.Label12.Size = new System.Drawing.Size(94, 13);
            this.Label12.TabIndex = 2;
            this.Label12.Text = "Audio Encoder:";
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
            // lbl_update
            // 
            this.lbl_update.AutoSize = true;
            this.lbl_update.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_update.ForeColor = System.Drawing.Color.Black;
            this.lbl_update.Location = new System.Drawing.Point(90, 551);
            this.lbl_update.Name = "lbl_update";
            this.lbl_update.Size = new System.Drawing.Size(193, 13);
            this.lbl_update.TabIndex = 7;
            this.lbl_update.Text = "- A New Version is available!";
            this.lbl_update.Visible = false;
            // 
            // btn_queue
            // 
            this.btn_queue.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_queue.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_queue.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_queue.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_queue.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_queue.Location = new System.Drawing.Point(414, 546);
            this.btn_queue.Name = "btn_queue";
            this.btn_queue.Size = new System.Drawing.Size(115, 22);
            this.btn_queue.TabIndex = 9;
            this.btn_queue.TabStop = false;
            this.btn_queue.Text = "Add to Queue";
            this.btn_queue.UseVisualStyleBackColor = false;
            this.btn_queue.Click += new System.EventHandler(this.btn_queue_Click);
            // 
            // btn_encode
            // 
            this.btn_encode.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_encode.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_encode.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_encode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_encode.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_encode.Location = new System.Drawing.Point(535, 546);
            this.btn_encode.Name = "btn_encode";
            this.btn_encode.Size = new System.Drawing.Size(124, 22);
            this.btn_encode.TabIndex = 10;
            this.btn_encode.TabStop = false;
            this.btn_encode.Text = "Encode Video";
            this.btn_encode.UseVisualStyleBackColor = false;
            this.btn_encode.Click += new System.EventHandler(this.btn_encode_Click);
            // 
            // Version
            // 
            this.Version.BackColor = System.Drawing.Color.Transparent;
            this.Version.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Version.Location = new System.Drawing.Point(11, 551);
            this.Version.Name = "Version";
            this.Version.Size = new System.Drawing.Size(105, 20);
            this.Version.TabIndex = 6;
            this.Version.Text = "Version 0.0.0";
            // 
            // lbl_encode
            // 
            this.lbl_encode.AutoSize = true;
            this.lbl_encode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_encode.Location = new System.Drawing.Point(11, 531);
            this.lbl_encode.Name = "lbl_encode";
            this.lbl_encode.Size = new System.Drawing.Size(129, 13);
            this.lbl_encode.TabIndex = 5;
            this.lbl_encode.Text = "Encoding started...";
            this.lbl_encode.Visible = false;
            // 
            // btn_eCancel
            // 
            this.btn_eCancel.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_eCancel.Enabled = false;
            this.btn_eCancel.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_eCancel.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_eCancel.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_eCancel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_eCancel.Location = new System.Drawing.Point(343, 546);
            this.btn_eCancel.Name = "btn_eCancel";
            this.btn_eCancel.Size = new System.Drawing.Size(65, 22);
            this.btn_eCancel.TabIndex = 8;
            this.btn_eCancel.TabStop = false;
            this.btn_eCancel.Text = "Cancel";
            this.btn_eCancel.UseVisualStyleBackColor = false;
            this.btn_eCancel.Visible = false;
            this.btn_eCancel.Click += new System.EventHandler(this.btn_eCancel_Click);
            // 
            // TabPage6
            // 
            this.TabPage6.BackColor = System.Drawing.SystemColors.ControlLight;
            this.TabPage6.Controls.Add(this.btn_copy);
            this.TabPage6.Controls.Add(this.label23);
            this.TabPage6.Controls.Add(this.Label7);
            this.TabPage6.Controls.Add(this.Label39);
            this.TabPage6.Controls.Add(this.btn_ClearQuery);
            this.TabPage6.Controls.Add(this.GenerateQuery);
            this.TabPage6.Controls.Add(this.QueryEditorText);
            this.TabPage6.Location = new System.Drawing.Point(4, 22);
            this.TabPage6.Name = "TabPage6";
            this.TabPage6.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage6.Size = new System.Drawing.Size(639, 226);
            this.TabPage6.TabIndex = 6;
            this.TabPage6.Text = "Query Editor";
            // 
            // btn_copy
            // 
            this.btn_copy.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_copy.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_copy.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_copy.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_copy.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_copy.Location = new System.Drawing.Point(176, 59);
            this.btn_copy.Name = "btn_copy";
            this.btn_copy.Size = new System.Drawing.Size(134, 23);
            this.btn_copy.TabIndex = 5;
            this.btn_copy.Text = "Copy to Clipboard";
            this.btn_copy.UseVisualStyleBackColor = false;
            this.btn_copy.Click += new System.EventHandler(this.btn_copy_Click);
            // 
            // label23
            // 
            this.label23.AutoSize = true;
            this.label23.BackColor = System.Drawing.Color.Transparent;
            this.label23.Location = new System.Drawing.Point(13, 186);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(403, 13);
            this.label23.TabIndex = 4;
            this.label23.Text = "Remember to re-generate the query each time you change a setting!";
            // 
            // Label7
            // 
            this.Label7.AutoSize = true;
            this.Label7.BackColor = System.Drawing.Color.Transparent;
            this.Label7.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label7.Location = new System.Drawing.Point(13, 13);
            this.Label7.Name = "Label7";
            this.Label7.Size = new System.Drawing.Size(89, 13);
            this.Label7.TabIndex = 0;
            this.Label7.Text = "Query Editor";
            // 
            // Label39
            // 
            this.Label39.AutoSize = true;
            this.Label39.BackColor = System.Drawing.Color.Transparent;
            this.Label39.Location = new System.Drawing.Point(13, 34);
            this.Label39.Name = "Label39";
            this.Label39.Size = new System.Drawing.Size(331, 13);
            this.Label39.TabIndex = 1;
            this.Label39.Text = "Here you can alter the query generated by the program.";
            // 
            // btn_ClearQuery
            // 
            this.btn_ClearQuery.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_ClearQuery.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_ClearQuery.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_ClearQuery.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_ClearQuery.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_ClearQuery.Location = new System.Drawing.Point(542, 59);
            this.btn_ClearQuery.Name = "btn_ClearQuery";
            this.btn_ClearQuery.Size = new System.Drawing.Size(79, 23);
            this.btn_ClearQuery.TabIndex = 6;
            this.btn_ClearQuery.Text = "Clear";
            this.btn_ClearQuery.UseVisualStyleBackColor = false;
            this.btn_ClearQuery.Click += new System.EventHandler(this.btn_ClearQuery_Click);
            // 
            // GenerateQuery
            // 
            this.GenerateQuery.BackColor = System.Drawing.SystemColors.ControlLight;
            this.GenerateQuery.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.GenerateQuery.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.GenerateQuery.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GenerateQuery.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.GenerateQuery.Location = new System.Drawing.Point(16, 59);
            this.GenerateQuery.Name = "GenerateQuery";
            this.GenerateQuery.Size = new System.Drawing.Size(154, 23);
            this.GenerateQuery.TabIndex = 2;
            this.GenerateQuery.Text = "Generate Query Now";
            this.GenerateQuery.UseVisualStyleBackColor = false;
            this.GenerateQuery.Click += new System.EventHandler(this.GenerateQuery_Click);
            // 
            // h264Tab
            // 
            this.h264Tab.BackColor = System.Drawing.SystemColors.ControlLight;
            this.h264Tab.Controls.Add(this.Label43);
            this.h264Tab.Controls.Add(this.label_h264);
            this.h264Tab.Controls.Add(this.Label95);
            this.h264Tab.Controls.Add(this.btn_h264Clear);
            this.h264Tab.Controls.Add(this.Label90);
            this.h264Tab.Controls.Add(this.rtf_h264advanced);
            this.h264Tab.Controls.Add(this.Label92);
            this.h264Tab.Location = new System.Drawing.Point(4, 22);
            this.h264Tab.Name = "h264Tab";
            this.h264Tab.Padding = new System.Windows.Forms.Padding(3);
            this.h264Tab.Size = new System.Drawing.Size(639, 226);
            this.h264Tab.TabIndex = 5;
            this.h264Tab.Text = "H.264";
            // 
            // Label43
            // 
            this.Label43.AutoSize = true;
            this.Label43.BackColor = System.Drawing.Color.Transparent;
            this.Label43.Location = new System.Drawing.Point(78, 187);
            this.Label43.Name = "Label43";
            this.Label43.Size = new System.Drawing.Size(158, 13);
            this.Label43.TabIndex = 6;
            this.Label43.Text = "for help using this feature.";
            // 
            // label_h264
            // 
            this.label_h264.AutoSize = true;
            this.label_h264.Location = new System.Drawing.Point(13, 187);
            this.label_h264.Name = "label_h264";
            this.label_h264.Size = new System.Drawing.Size(66, 13);
            this.label_h264.TabIndex = 5;
            this.label_h264.TabStop = true;
            this.label_h264.Text = "Click Here";
            this.label_h264.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.label_h264_LinkClicked);
            // 
            // Label95
            // 
            this.Label95.AutoSize = true;
            this.Label95.BackColor = System.Drawing.Color.Transparent;
            this.Label95.Location = new System.Drawing.Point(13, 168);
            this.Label95.Name = "Label95";
            this.Label95.Size = new System.Drawing.Size(387, 13);
            this.Label95.TabIndex = 4;
            this.Label95.Text = "Note: Incorrect usage of this feature will cause the encoder to fail!";
            // 
            // btn_h264Clear
            // 
            this.btn_h264Clear.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_h264Clear.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_h264Clear.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_h264Clear.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_h264Clear.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_h264Clear.Location = new System.Drawing.Point(542, 50);
            this.btn_h264Clear.Name = "btn_h264Clear";
            this.btn_h264Clear.Size = new System.Drawing.Size(79, 23);
            this.btn_h264Clear.TabIndex = 3;
            this.btn_h264Clear.Text = "Clear";
            this.btn_h264Clear.UseVisualStyleBackColor = false;
            this.btn_h264Clear.Click += new System.EventHandler(this.btn_h264Clear_Click);
            // 
            // Label90
            // 
            this.Label90.AutoSize = true;
            this.Label90.BackColor = System.Drawing.Color.Transparent;
            this.Label90.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label90.Location = new System.Drawing.Point(13, 13);
            this.Label90.Name = "Label90";
            this.Label90.Size = new System.Drawing.Size(165, 13);
            this.Label90.TabIndex = 0;
            this.Label90.Text = "Advanced H.264 Options";
            // 
            // Label92
            // 
            this.Label92.AutoSize = true;
            this.Label92.BackColor = System.Drawing.Color.Transparent;
            this.Label92.Location = new System.Drawing.Point(13, 41);
            this.Label92.Name = "Label92";
            this.Label92.Size = new System.Drawing.Size(370, 26);
            this.Label92.TabIndex = 1;
            this.Label92.Text = "Specify advanced x264 options in the same style as mencoder:\r\noption1=value1:opti" +
                "on2=value2";
            // 
            // TabPage2
            // 
            this.TabPage2.BackColor = System.Drawing.SystemColors.ControlLight;
            this.TabPage2.Controls.Add(this.drp_subtitle);
            this.TabPage2.Controls.Add(this.Label19);
            this.TabPage2.Controls.Add(this.Label21);
            this.TabPage2.Controls.Add(this.Label20);
            this.TabPage2.Controls.Add(this.Label29);
            this.TabPage2.Controls.Add(this.drp_audioMixDown);
            this.TabPage2.Controls.Add(this.drp_audioChannels);
            this.TabPage2.Controls.Add(this.drp_audioBitrate);
            this.TabPage2.Controls.Add(this.Label14);
            this.TabPage2.Controls.Add(this.Label5);
            this.TabPage2.Controls.Add(this.Label16);
            this.TabPage2.Controls.Add(this.Label32);
            this.TabPage2.Controls.Add(this.Label18);
            this.TabPage2.Controls.Add(this.drp_audioSampleRate);
            this.TabPage2.Location = new System.Drawing.Point(4, 22);
            this.TabPage2.Name = "TabPage2";
            this.TabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage2.Size = new System.Drawing.Size(639, 226);
            this.TabPage2.TabIndex = 3;
            this.TabPage2.Text = "Audio && Subtitles";
            // 
            // Label19
            // 
            this.Label19.AutoSize = true;
            this.Label19.BackColor = System.Drawing.Color.Transparent;
            this.Label19.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label19.Location = new System.Drawing.Point(339, 13);
            this.Label19.Name = "Label19";
            this.Label19.Size = new System.Drawing.Size(64, 13);
            this.Label19.TabIndex = 10;
            this.Label19.Text = "Subtitles";
            // 
            // Label21
            // 
            this.Label21.AutoSize = true;
            this.Label21.BackColor = System.Drawing.Color.Transparent;
            this.Label21.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label21.Location = new System.Drawing.Point(403, 63);
            this.Label21.Name = "Label21";
            this.Label21.Size = new System.Drawing.Size(224, 26);
            this.Label21.TabIndex = 13;
            this.Label21.Text = "Please note that subtitles will be hard \r\ncoded into the video.\r\n";
            // 
            // Label20
            // 
            this.Label20.AutoSize = true;
            this.Label20.BackColor = System.Drawing.Color.Transparent;
            this.Label20.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label20.Location = new System.Drawing.Point(339, 39);
            this.Label20.Name = "Label20";
            this.Label20.Size = new System.Drawing.Size(61, 13);
            this.Label20.TabIndex = 11;
            this.Label20.Text = "Subtitles:";
            // 
            // Label29
            // 
            this.Label29.AutoSize = true;
            this.Label29.BackColor = System.Drawing.Color.Transparent;
            this.Label29.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label29.Location = new System.Drawing.Point(135, 151);
            this.Label29.Name = "Label29";
            this.Label29.Size = new System.Drawing.Size(189, 24);
            this.Label29.TabIndex = 9;
            this.Label29.Text = "Please note: Some options require a \r\n5.1 Audio Channel to be selected.";
            // 
            // drp_audioMixDown
            // 
            this.drp_audioMixDown.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audioMixDown.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audioMixDown.FormattingEnabled = true;
            this.drp_audioMixDown.Items.AddRange(new object[] {
            "Automatic",
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audioMixDown.Location = new System.Drawing.Point(137, 127);
            this.drp_audioMixDown.Name = "drp_audioMixDown";
            this.drp_audioMixDown.Size = new System.Drawing.Size(173, 21);
            this.drp_audioMixDown.TabIndex = 8;
            this.drp_audioMixDown.Text = "Automatic";
            this.drp_audioMixDown.SelectedIndexChanged += new System.EventHandler(this.drp_audioMixDown_SelectedIndexChanged);
            // 
            // drp_audioChannels
            // 
            this.drp_audioChannels.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audioChannels.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audioChannels.FormattingEnabled = true;
            this.drp_audioChannels.Location = new System.Drawing.Point(137, 97);
            this.drp_audioChannels.Name = "drp_audioChannels";
            this.drp_audioChannels.Size = new System.Drawing.Size(173, 21);
            this.drp_audioChannels.TabIndex = 7;
            this.drp_audioChannels.Text = "Automatic";
            // 
            // Label14
            // 
            this.Label14.AutoSize = true;
            this.Label14.BackColor = System.Drawing.Color.Transparent;
            this.Label14.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label14.Location = new System.Drawing.Point(13, 130);
            this.Label14.Name = "Label14";
            this.Label14.Size = new System.Drawing.Size(78, 13);
            this.Label14.TabIndex = 4;
            this.Label14.Text = "Track 1 Mix:";
            // 
            // Label5
            // 
            this.Label5.AutoSize = true;
            this.Label5.BackColor = System.Drawing.Color.Transparent;
            this.Label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label5.Location = new System.Drawing.Point(13, 13);
            this.Label5.Name = "Label5";
            this.Label5.Size = new System.Drawing.Size(101, 13);
            this.Label5.TabIndex = 0;
            this.Label5.Text = "Audio Settings";
            // 
            // Label16
            // 
            this.Label16.AutoSize = true;
            this.Label16.BackColor = System.Drawing.Color.Transparent;
            this.Label16.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label16.Location = new System.Drawing.Point(13, 40);
            this.Label16.Name = "Label16";
            this.Label16.Size = new System.Drawing.Size(91, 13);
            this.Label16.TabIndex = 1;
            this.Label16.Text = "Bitrate (kbps):";
            // 
            // Label32
            // 
            this.Label32.AutoSize = true;
            this.Label32.BackColor = System.Drawing.Color.Transparent;
            this.Label32.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label32.Location = new System.Drawing.Point(13, 100);
            this.Label32.Name = "Label32";
            this.Label32.Size = new System.Drawing.Size(55, 13);
            this.Label32.TabIndex = 3;
            this.Label32.Text = "Track 1:";
            // 
            // Label18
            // 
            this.Label18.AutoSize = true;
            this.Label18.BackColor = System.Drawing.Color.Transparent;
            this.Label18.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label18.Location = new System.Drawing.Point(13, 70);
            this.Label18.Name = "Label18";
            this.Label18.Size = new System.Drawing.Size(120, 13);
            this.Label18.TabIndex = 2;
            this.Label18.Text = "Sample Rate (kHz):";
            // 
            // TabPage3
            // 
            this.TabPage3.BackColor = System.Drawing.SystemColors.ControlLight;
            this.TabPage3.Controls.Add(this.lbl_vfr);
            this.TabPage3.Controls.Add(this.check_iPodAtom);
            this.TabPage3.Controls.Add(this.check_optimiseMP4);
            this.TabPage3.Controls.Add(this.check_grayscale);
            this.TabPage3.Controls.Add(this.lbl_largeMp4Warning);
            this.TabPage3.Controls.Add(this.check_largeFile);
            this.TabPage3.Controls.Add(this.check_turbo);
            this.TabPage3.Controls.Add(this.CheckCRF);
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
            this.TabPage3.Size = new System.Drawing.Size(639, 226);
            this.TabPage3.TabIndex = 2;
            this.TabPage3.Text = "Video Settings";
            // 
            // check_iPodAtom
            // 
            this.check_iPodAtom.AutoSize = true;
            this.check_iPodAtom.BackColor = System.Drawing.Color.DarkRed;
            this.check_iPodAtom.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_iPodAtom.Location = new System.Drawing.Point(16, 167);
            this.check_iPodAtom.Name = "check_iPodAtom";
            this.check_iPodAtom.Size = new System.Drawing.Size(110, 17);
            this.check_iPodAtom.TabIndex = 26;
            this.check_iPodAtom.Text = "Add iPod Atom";
            this.check_iPodAtom.UseVisualStyleBackColor = false;
            // 
            // check_optimiseMP4
            // 
            this.check_optimiseMP4.AutoSize = true;
            this.check_optimiseMP4.BackColor = System.Drawing.Color.DarkRed;
            this.check_optimiseMP4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_optimiseMP4.Location = new System.Drawing.Point(16, 144);
            this.check_optimiseMP4.Name = "check_optimiseMP4";
            this.check_optimiseMP4.Size = new System.Drawing.Size(184, 17);
            this.check_optimiseMP4.TabIndex = 25;
            this.check_optimiseMP4.Text = "Optimize MP4 for streaming";
            this.check_optimiseMP4.UseVisualStyleBackColor = false;
            // 
            // check_grayscale
            // 
            this.check_grayscale.AutoSize = true;
            this.check_grayscale.BackColor = System.Drawing.Color.Transparent;
            this.check_grayscale.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_grayscale.Location = new System.Drawing.Point(16, 38);
            this.check_grayscale.Name = "check_grayscale";
            this.check_grayscale.Size = new System.Drawing.Size(138, 17);
            this.check_grayscale.TabIndex = 1;
            this.check_grayscale.Text = "Grayscale Encoding";
            this.check_grayscale.UseVisualStyleBackColor = false;
            // 
            // lbl_largeMp4Warning
            // 
            this.lbl_largeMp4Warning.AutoSize = true;
            this.lbl_largeMp4Warning.BackColor = System.Drawing.Color.Transparent;
            this.lbl_largeMp4Warning.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_largeMp4Warning.Location = new System.Drawing.Point(35, 127);
            this.lbl_largeMp4Warning.Name = "lbl_largeMp4Warning";
            this.lbl_largeMp4Warning.Size = new System.Drawing.Size(241, 12);
            this.lbl_largeMp4Warning.TabIndex = 5;
            this.lbl_largeMp4Warning.Text = "Warning: Breaks iPod, @TV, PS3 compatibility.";
            // 
            // Label22
            // 
            this.Label22.AutoSize = true;
            this.Label22.BackColor = System.Drawing.Color.Transparent;
            this.Label22.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label22.Location = new System.Drawing.Point(13, 13);
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
            this.check_2PassEncode.Location = new System.Drawing.Point(16, 61);
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
            this.Label2.Size = new System.Drawing.Size(110, 13);
            this.Label2.TabIndex = 8;
            this.Label2.Text = "Quality Settings";
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
            this.Label46.Location = new System.Drawing.Point(13, 192);
            this.Label46.Name = "Label46";
            this.Label46.Size = new System.Drawing.Size(107, 13);
            this.Label46.TabIndex = 6;
            this.Label46.Text = "Video Framerate:";
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
            this.TabPage1.BackColor = System.Drawing.SystemColors.ControlLight;
            this.TabPage1.Controls.Add(this.check_lAnamorphic);
            this.TabPage1.Controls.Add(this.check_vfr);
            this.TabPage1.Controls.Add(this.label24);
            this.TabPage1.Controls.Add(this.drp_deNoise);
            this.TabPage1.Controls.Add(this.label11);
            this.TabPage1.Controls.Add(this.check_deblock);
            this.TabPage1.Controls.Add(this.check_detelecine);
            this.TabPage1.Controls.Add(this.label4);
            this.TabPage1.Controls.Add(this.drp_deInterlace_option);
            this.TabPage1.Controls.Add(this.Check_ChapterMarkers);
            this.TabPage1.Controls.Add(this.CheckPixelRatio);
            this.TabPage1.Controls.Add(this.label6);
            this.TabPage1.Controls.Add(this.lbl_RecomendedCrop);
            this.TabPage1.Controls.Add(this.Label8);
            this.TabPage1.Controls.Add(this.Label1);
            this.TabPage1.Controls.Add(this.Label53);
            this.TabPage1.Controls.Add(this.Label52);
            this.TabPage1.Controls.Add(this.Label51);
            this.TabPage1.Controls.Add(this.Label50);
            this.TabPage1.Controls.Add(this.Label15);
            this.TabPage1.Controls.Add(this.text_top);
            this.TabPage1.Controls.Add(this.text_bottom);
            this.TabPage1.Controls.Add(this.drp_crop);
            this.TabPage1.Controls.Add(this.text_right);
            this.TabPage1.Controls.Add(this.text_left);
            this.TabPage1.Location = new System.Drawing.Point(4, 22);
            this.TabPage1.Name = "TabPage1";
            this.TabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage1.Size = new System.Drawing.Size(639, 226);
            this.TabPage1.TabIndex = 0;
            this.TabPage1.Text = "Picture Settings";
            // 
            // check_lAnamorphic
            // 
            this.check_lAnamorphic.AutoSize = true;
            this.check_lAnamorphic.BackColor = System.Drawing.Color.Transparent;
            this.check_lAnamorphic.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_lAnamorphic.Location = new System.Drawing.Point(440, 166);
            this.check_lAnamorphic.Name = "check_lAnamorphic";
            this.check_lAnamorphic.Size = new System.Drawing.Size(131, 17);
            this.check_lAnamorphic.TabIndex = 24;
            this.check_lAnamorphic.Text = "Loose Anamorphic";
            this.check_lAnamorphic.UseVisualStyleBackColor = false;
            this.check_lAnamorphic.CheckedChanged += new System.EventHandler(this.check_lAnamorphic_CheckedChanged);
            // 
            // check_vfr
            // 
            this.check_vfr.AutoSize = true;
            this.check_vfr.BackColor = System.Drawing.Color.Transparent;
            this.check_vfr.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_vfr.Location = new System.Drawing.Point(416, 36);
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
            this.label24.Location = new System.Drawing.Point(314, 13);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(109, 13);
            this.label24.TabIndex = 13;
            this.label24.Text = "Picture Cleanup";
            // 
            // drp_deNoise
            // 
            this.drp_deNoise.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_deNoise.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_deNoise.FormattingEnabled = true;
            this.drp_deNoise.Items.AddRange(new object[] {
            "None",
            "Weak",
            "Medium",
            "Strong"});
            this.drp_deNoise.Location = new System.Drawing.Point(416, 111);
            this.drp_deNoise.Name = "drp_deNoise";
            this.drp_deNoise.Size = new System.Drawing.Size(161, 21);
            this.drp_deNoise.TabIndex = 19;
            this.drp_deNoise.Text = "None";
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.BackColor = System.Drawing.Color.Transparent;
            this.label11.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(314, 114);
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
            this.check_deblock.Location = new System.Drawing.Point(317, 59);
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
            this.check_detelecine.Location = new System.Drawing.Point(317, 36);
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
            this.label4.Location = new System.Drawing.Point(314, 86);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(77, 13);
            this.label4.TabIndex = 16;
            this.label4.Text = "Deinterlace:";
            // 
            // drp_deInterlace_option
            // 
            this.drp_deInterlace_option.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_deInterlace_option.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_deInterlace_option.FormattingEnabled = true;
            this.drp_deInterlace_option.Items.AddRange(new object[] {
            "None",
            "Original (Fast)",
            "yadif (Slow)",
            "yadif + mcdeint (Slower)",
            "yadif + mcdeint (Slowest)"});
            this.drp_deInterlace_option.Location = new System.Drawing.Point(416, 83);
            this.drp_deInterlace_option.Name = "drp_deInterlace_option";
            this.drp_deInterlace_option.Size = new System.Drawing.Size(161, 21);
            this.drp_deInterlace_option.TabIndex = 17;
            this.drp_deInterlace_option.Text = "None";
            // 
            // Check_ChapterMarkers
            // 
            this.Check_ChapterMarkers.AutoSize = true;
            this.Check_ChapterMarkers.BackColor = System.Drawing.Color.Transparent;
            this.Check_ChapterMarkers.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Check_ChapterMarkers.Location = new System.Drawing.Point(313, 189);
            this.Check_ChapterMarkers.Name = "Check_ChapterMarkers";
            this.Check_ChapterMarkers.Size = new System.Drawing.Size(122, 17);
            this.Check_ChapterMarkers.TabIndex = 22;
            this.Check_ChapterMarkers.Text = "Chapter Markers";
            this.Check_ChapterMarkers.UseVisualStyleBackColor = false;
            this.Check_ChapterMarkers.CheckedChanged += new System.EventHandler(this.Check_ChapterMarkers_CheckedChanged);
            // 
            // CheckPixelRatio
            // 
            this.CheckPixelRatio.AutoSize = true;
            this.CheckPixelRatio.BackColor = System.Drawing.Color.Transparent;
            this.CheckPixelRatio.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.CheckPixelRatio.Location = new System.Drawing.Point(313, 166);
            this.CheckPixelRatio.Name = "CheckPixelRatio";
            this.CheckPixelRatio.Size = new System.Drawing.Size(121, 17);
            this.CheckPixelRatio.TabIndex = 21;
            this.CheckPixelRatio.Text = "Anamorphic PAR";
            this.CheckPixelRatio.UseVisualStyleBackColor = false;
            this.CheckPixelRatio.CheckedChanged += new System.EventHandler(this.CheckPixelRatio_CheckedChanged);
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.BackColor = System.Drawing.Color.Transparent;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(310, 144);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(173, 13);
            this.label6.TabIndex = 20;
            this.label6.Text = "Additional Output Options";
            // 
            // lbl_RecomendedCrop
            // 
            this.lbl_RecomendedCrop.AutoSize = true;
            this.lbl_RecomendedCrop.BackColor = System.Drawing.Color.Transparent;
            this.lbl_RecomendedCrop.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_RecomendedCrop.Location = new System.Drawing.Point(119, 72);
            this.lbl_RecomendedCrop.Name = "lbl_RecomendedCrop";
            this.lbl_RecomendedCrop.Size = new System.Drawing.Size(72, 12);
            this.lbl_RecomendedCrop.TabIndex = 4;
            this.lbl_RecomendedCrop.Text = "Select a Title";
            // 
            // Label8
            // 
            this.Label8.AutoSize = true;
            this.Label8.BackColor = System.Drawing.Color.Transparent;
            this.Label8.Location = new System.Drawing.Point(13, 71);
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
            this.Label1.Size = new System.Drawing.Size(65, 13);
            this.Label1.TabIndex = 0;
            this.Label1.Text = "Cropping";
            // 
            // Label53
            // 
            this.Label53.AutoSize = true;
            this.Label53.BackColor = System.Drawing.Color.Transparent;
            this.Label53.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label53.Location = new System.Drawing.Point(134, 187);
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
            this.Label52.Location = new System.Drawing.Point(138, 96);
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
            this.Label51.Location = new System.Drawing.Point(247, 139);
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
            this.Label50.Location = new System.Drawing.Point(13, 41);
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
            this.Label15.Location = new System.Drawing.Point(39, 139);
            this.Label15.Name = "Label15";
            this.Label15.Size = new System.Drawing.Size(28, 13);
            this.Label15.TabIndex = 7;
            this.Label15.Text = "Left";
            // 
            // text_top
            // 
            this.text_top.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_top.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_top.Location = new System.Drawing.Point(131, 111);
            this.text_top.Name = "text_top";
            this.text_top.Size = new System.Drawing.Size(51, 21);
            this.text_top.TabIndex = 6;
            this.text_top.Text = "0";
            // 
            // text_bottom
            // 
            this.text_bottom.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_bottom.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_bottom.Location = new System.Drawing.Point(131, 163);
            this.text_bottom.Name = "text_bottom";
            this.text_bottom.Size = new System.Drawing.Size(51, 21);
            this.text_bottom.TabIndex = 9;
            this.text_bottom.Text = "0";
            // 
            // drp_crop
            // 
            this.drp_crop.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_crop.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_crop.FormattingEnabled = true;
            this.drp_crop.Items.AddRange(new object[] {
            "Auto Crop",
            "No Crop",
            "Manual"});
            this.drp_crop.Location = new System.Drawing.Point(118, 36);
            this.drp_crop.Name = "drp_crop";
            this.drp_crop.Size = new System.Drawing.Size(123, 21);
            this.drp_crop.TabIndex = 3;
            this.drp_crop.Text = "No Crop";
            this.drp_crop.SelectedIndexChanged += new System.EventHandler(this.drp_crop_SelectedIndexChanged);
            // 
            // text_right
            // 
            this.text_right.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_right.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_right.Location = new System.Drawing.Point(190, 136);
            this.text_right.Name = "text_right";
            this.text_right.Size = new System.Drawing.Size(51, 21);
            this.text_right.TabIndex = 11;
            this.text_right.Text = "0";
            // 
            // text_left
            // 
            this.text_left.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_left.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_left.Location = new System.Drawing.Point(72, 136);
            this.text_left.Name = "text_left";
            this.text_left.Size = new System.Drawing.Size(51, 21);
            this.text_left.TabIndex = 8;
            this.text_left.Text = "0";
            // 
            // advancedOptions
            // 
            this.advancedOptions.Controls.Add(this.TabPage1);
            this.advancedOptions.Controls.Add(this.TabPage3);
            this.advancedOptions.Controls.Add(this.TabPage2);
            this.advancedOptions.Controls.Add(this.h264Tab);
            this.advancedOptions.Controls.Add(this.TabPage6);
            this.advancedOptions.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.advancedOptions.Location = new System.Drawing.Point(14, 276);
            this.advancedOptions.Name = "advancedOptions";
            this.advancedOptions.SelectedIndex = 0;
            this.advancedOptions.Size = new System.Drawing.Size(647, 252);
            this.advancedOptions.TabIndex = 4;
            this.advancedOptions.TabStop = false;
            // 
            // groupBox_dest
            // 
            this.groupBox_dest.Controls.Add(this.Label3);
            this.groupBox_dest.Controls.Add(this.text_destination);
            this.groupBox_dest.Controls.Add(this.btn_destBrowse);
            this.groupBox_dest.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox_dest.ForeColor = System.Drawing.Color.Black;
            this.groupBox_dest.Location = new System.Drawing.Point(14, 128);
            this.groupBox_dest.Name = "groupBox_dest";
            this.groupBox_dest.Size = new System.Drawing.Size(646, 50);
            this.groupBox_dest.TabIndex = 2;
            this.groupBox_dest.TabStop = false;
            this.groupBox_dest.Text = "Destination";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.treeView_presets);
            this.groupBox2.Controls.Add(this.btn_setDefault);
            this.groupBox2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox2.ForeColor = System.Drawing.Color.Black;
            this.groupBox2.Location = new System.Drawing.Point(674, 35);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(193, 482);
            this.groupBox2.TabIndex = 11;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Presets";
            // 
            // treeView_presets
            // 
            this.treeView_presets.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.treeView_presets.ForeColor = System.Drawing.Color.Navy;
            this.treeView_presets.FullRowSelect = true;
            this.treeView_presets.HideSelection = false;
            this.treeView_presets.ItemHeight = 17;
            this.treeView_presets.Location = new System.Drawing.Point(8, 19);
            this.treeView_presets.Name = "treeView_presets";
            this.treeView_presets.ShowLines = false;
            this.treeView_presets.Size = new System.Drawing.Size(177, 421);
            this.treeView_presets.TabIndex = 2;
            this.treeView_presets.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView_presets_AfterSelect);
            // 
            // lbl_vfr
            // 
            this.lbl_vfr.AutoSize = true;
            this.lbl_vfr.BackColor = System.Drawing.Color.Transparent;
            this.lbl_vfr.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_vfr.Location = new System.Drawing.Point(220, 193);
            this.lbl_vfr.Name = "lbl_vfr";
            this.lbl_vfr.Size = new System.Drawing.Size(52, 12);
            this.lbl_vfr.TabIndex = 27;
            this.lbl_vfr.Text = "(VFR On)";
            this.lbl_vfr.Visible = false;
            // 
            // frmMain
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(874, 574);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox_dest);
            this.Controls.Add(this.btn_eCancel);
            this.Controls.Add(this.lbl_encode);
            this.Controls.Add(this.lbl_update);
            this.Controls.Add(this.btn_queue);
            this.Controls.Add(this.btn_encode);
            this.Controls.Add(this.Version);
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
            this.frmMainMenu.ResumeLayout(false);
            this.frmMainMenu.PerformLayout();
            this.GroupBox1.ResumeLayout(false);
            this.GroupBox1.PerformLayout();
            this.groupBox_output.ResumeLayout(false);
            this.groupBox_output.PerformLayout();
            this.TabPage6.ResumeLayout(false);
            this.TabPage6.PerformLayout();
            this.h264Tab.ResumeLayout(false);
            this.h264Tab.PerformLayout();
            this.TabPage2.ResumeLayout(false);
            this.TabPage2.PerformLayout();
            this.TabPage3.ResumeLayout(false);
            this.TabPage3.PerformLayout();
            this.TabPage1.ResumeLayout(false);
            this.TabPage1.PerformLayout();
            this.advancedOptions.ResumeLayout(false);
            this.groupBox_dest.ResumeLayout(false);
            this.groupBox_dest.PerformLayout();
            this.groupBox2.ResumeLayout(false);
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
        internal System.Windows.Forms.ToolStripMenuItem WebsiteToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_homepage;
        internal System.Windows.Forms.ToolStripMenuItem mnu_forum;
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator3;
        internal System.Windows.Forms.ToolStripMenuItem mnu_about;
        internal System.Windows.Forms.MenuStrip frmMainMenu;
        internal System.Windows.Forms.GroupBox GroupBox1;
        internal System.Windows.Forms.Label Label13;
        internal System.Windows.Forms.ComboBox drop_chapterFinish;
        internal System.Windows.Forms.ComboBox drop_chapterStart;
        internal System.Windows.Forms.ComboBox drp_dvdtitle;
        internal System.Windows.Forms.RadioButton RadioDVD;
        internal System.Windows.Forms.RadioButton RadioISO;
        internal System.Windows.Forms.Button btn_Browse;
        internal System.Windows.Forms.Label Label17;
        internal System.Windows.Forms.TextBox text_source;
        internal System.Windows.Forms.Label Label9;
        internal System.Windows.Forms.Label Label10;
        internal System.Windows.Forms.GroupBox groupBox_output;
        internal System.Windows.Forms.Label Label56;
        internal System.Windows.Forms.Label lbl_Aspect;
        internal System.Windows.Forms.Label Label91;
        internal System.Windows.Forms.TextBox text_height;
        internal System.Windows.Forms.Label Label55;
        internal System.Windows.Forms.TextBox text_width;
        internal System.Windows.Forms.Button btn_destBrowse;
        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.ComboBox drp_videoEncoder;
        internal System.Windows.Forms.Label Label47;
        internal System.Windows.Forms.TextBox text_destination;
        internal System.Windows.Forms.ComboBox drp_audioCodec;
        internal System.Windows.Forms.Label Label12;
        internal System.Windows.Forms.Label lbl_update;
        internal System.Windows.Forms.Button btn_queue;
        internal System.Windows.Forms.Button btn_encode;
        internal System.Windows.Forms.Label Version;
        private System.Windows.Forms.Label lbl_encode;
        internal System.Windows.Forms.Button btn_eCancel;
        internal System.Windows.Forms.TabPage TabPage6;
        internal System.Windows.Forms.Label Label7;
        internal System.Windows.Forms.Label Label39;
        internal System.Windows.Forms.Button btn_ClearQuery;
        internal System.Windows.Forms.Button GenerateQuery;
        internal System.Windows.Forms.RichTextBox QueryEditorText;
        internal System.Windows.Forms.TabPage h264Tab;
        internal System.Windows.Forms.Label Label43;
        internal System.Windows.Forms.LinkLabel label_h264;
        internal System.Windows.Forms.Label Label95;
        internal System.Windows.Forms.Button btn_h264Clear;
        internal System.Windows.Forms.Label Label90;
        internal System.Windows.Forms.RichTextBox rtf_h264advanced;
        internal System.Windows.Forms.Label Label92;
        internal System.Windows.Forms.TabPage TabPage2;
        internal System.Windows.Forms.Label Label29;
        internal System.Windows.Forms.ComboBox drp_audioMixDown;
        internal System.Windows.Forms.ComboBox drp_audioChannels;
        internal System.Windows.Forms.ComboBox drp_audioBitrate;
        internal System.Windows.Forms.Label Label14;
        internal System.Windows.Forms.Label Label5;
        internal System.Windows.Forms.Label Label16;
        internal System.Windows.Forms.Label Label32;
        internal System.Windows.Forms.Label Label18;
        internal System.Windows.Forms.ComboBox drp_audioSampleRate;
        internal System.Windows.Forms.TabPage TabPage3;
        internal System.Windows.Forms.Label lbl_largeMp4Warning;
        internal System.Windows.Forms.CheckBox check_largeFile;
        internal System.Windows.Forms.CheckBox check_turbo;
        internal System.Windows.Forms.CheckBox CheckCRF;
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
        internal System.Windows.Forms.CheckBox CheckPixelRatio;
        internal System.Windows.Forms.Label label6;
        internal System.Windows.Forms.Label lbl_RecomendedCrop;
        internal System.Windows.Forms.Label Label8;
        internal System.Windows.Forms.Label Label1;
        internal System.Windows.Forms.Label Label53;
        internal System.Windows.Forms.Label Label52;
        internal System.Windows.Forms.Label Label51;
        internal System.Windows.Forms.Label Label50;
        internal System.Windows.Forms.Label Label15;
        internal System.Windows.Forms.TextBox text_top;
        internal System.Windows.Forms.TextBox text_bottom;
        internal System.Windows.Forms.ComboBox drp_crop;
        internal System.Windows.Forms.TextBox text_right;
        internal System.Windows.Forms.TextBox text_left;
        internal System.Windows.Forms.TabControl advancedOptions;
        internal System.Windows.Forms.Label Label46;
        private System.Windows.Forms.GroupBox groupBox_dest;
        internal System.Windows.Forms.ComboBox drp_subtitle;
        internal System.Windows.Forms.Label Label19;
        internal System.Windows.Forms.Label Label21;
        internal System.Windows.Forms.Label Label20;
        internal System.Windows.Forms.CheckBox check_grayscale;
        internal System.Windows.Forms.Label label24;
        internal System.Windows.Forms.ComboBox drp_deNoise;
        internal System.Windows.Forms.Label label11;
        internal System.Windows.Forms.CheckBox check_deblock;
        internal System.Windows.Forms.CheckBox check_detelecine;
        internal System.Windows.Forms.Label label4;
        internal System.Windows.Forms.ComboBox drp_deInterlace_option;
        private System.Windows.Forms.Label label23;
        internal System.Windows.Forms.Button btn_copy;
        private System.Windows.Forms.GroupBox groupBox2;
        internal System.Windows.Forms.Button btn_setDefault;
        private System.Windows.Forms.ToolStripMenuItem mnu_SelectDefault;
        private System.Windows.Forms.ToolStripMenuItem mnu_UpdateCheck;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator6;
        private System.Windows.Forms.ToolStripMenuItem mnu_quickStart;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.SaveFileDialog DVD_Save;
        private System.Windows.Forms.OpenFileDialog File_Open;
        private System.Windows.Forms.OpenFileDialog ISO_Open;
        private System.Windows.Forms.FolderBrowserDialog DVD_Open;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem mnu_open;
        private System.Windows.Forms.ToolStripMenuItem mnu_save;
        private System.Windows.Forms.ToolStripMenuItem mnu_showPresets;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator7;
        private System.Windows.Forms.TreeView treeView_presets;
        internal System.Windows.Forms.CheckBox check_lAnamorphic;
        internal System.Windows.Forms.CheckBox check_vfr;
        internal System.Windows.Forms.CheckBox check_iPodAtom;
        private System.Windows.Forms.CheckBox check_optimiseMP4;
        internal System.Windows.Forms.Label lbl_vfr;

    }
}