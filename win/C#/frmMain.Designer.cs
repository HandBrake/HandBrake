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
            this.drp_subtitle = new System.Windows.Forms.ComboBox();
            this.DVD_Open = new System.Windows.Forms.FolderBrowserDialog();
            this.File_Open = new System.Windows.Forms.OpenFileDialog();
            this.ISO_Open = new System.Windows.Forms.OpenFileDialog();
            this.FileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_open = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_save = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_update = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_exit = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_encode = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_viewDVDdata = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_options = new System.Windows.Forms.ToolStripMenuItem();
            this.PresetsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_preset_ipod133 = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_preset_ipod178 = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_preset_ipod235 = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_appleTv = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_presetPS3 = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_ProgramDefaultOptions = new System.Windows.Forms.ToolStripMenuItem();
            this.HelpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.OnlineDocumentationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_wiki = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_onlineDocs = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_faq = new System.Windows.Forms.ToolStripMenuItem();
            this.WebsiteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_homepage = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_forum = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_about = new System.Windows.Forms.ToolStripMenuItem();
            this.frmMainMenu = new System.Windows.Forms.MenuStrip();
            this.GroupBox1 = new System.Windows.Forms.GroupBox();
            this.Label13 = new System.Windows.Forms.Label();
            this.RadioDVD = new System.Windows.Forms.RadioButton();
            this.btn_Browse = new System.Windows.Forms.Button();
            this.Label17 = new System.Windows.Forms.Label();
            this.Label9 = new System.Windows.Forms.Label();
            this.Label10 = new System.Windows.Forms.Label();
            this.GroupBox4 = new System.Windows.Forms.GroupBox();
            this.Label56 = new System.Windows.Forms.Label();
            this.lbl_Aspect = new System.Windows.Forms.Label();
            this.Label91 = new System.Windows.Forms.Label();
            this.text_height = new System.Windows.Forms.TextBox();
            this.Label55 = new System.Windows.Forms.Label();
            this.text_width = new System.Windows.Forms.TextBox();
            this.btn_destBrowse = new System.Windows.Forms.Button();
            this.Label3 = new System.Windows.Forms.Label();
            this.drp_videoEncoder = new System.Windows.Forms.ComboBox();
            this.Label47 = new System.Windows.Forms.Label();
            this.drp_audioCodec = new System.Windows.Forms.ComboBox();
            this.Label12 = new System.Windows.Forms.Label();
            this.advancedOptions = new System.Windows.Forms.TabControl();
            this.TabPage1 = new System.Windows.Forms.TabPage();
            this.Label19 = new System.Windows.Forms.Label();
            this.lbl_RecomendedCrop = new System.Windows.Forms.Label();
            this.Label8 = new System.Windows.Forms.Label();
            this.Label1 = new System.Windows.Forms.Label();
            this.Label53 = new System.Windows.Forms.Label();
            this.Label21 = new System.Windows.Forms.Label();
            this.Label20 = new System.Windows.Forms.Label();
            this.Label52 = new System.Windows.Forms.Label();
            this.Label51 = new System.Windows.Forms.Label();
            this.Label50 = new System.Windows.Forms.Label();
            this.Label15 = new System.Windows.Forms.Label();
            this.text_top = new System.Windows.Forms.TextBox();
            this.text_bottom = new System.Windows.Forms.TextBox();
            this.drp_crop = new System.Windows.Forms.ComboBox();
            this.text_right = new System.Windows.Forms.TextBox();
            this.text_left = new System.Windows.Forms.TextBox();
            this.TabPage3 = new System.Windows.Forms.TabPage();
            this.Label41 = new System.Windows.Forms.Label();
            this.Label37 = new System.Windows.Forms.Label();
            this.check_largeFile = new System.Windows.Forms.CheckBox();
            this.check_turbo = new System.Windows.Forms.CheckBox();
            this.Label36 = new System.Windows.Forms.Label();
            this.Check_ChapterMarkers = new System.Windows.Forms.CheckBox();
            this.Label28 = new System.Windows.Forms.Label();
            this.Label27 = new System.Windows.Forms.Label();
            this.Label4 = new System.Windows.Forms.Label();
            this.CheckCRF = new System.Windows.Forms.CheckBox();
            this.CheckPixelRatio = new System.Windows.Forms.CheckBox();
            this.Label23 = new System.Windows.Forms.Label();
            this.Label22 = new System.Windows.Forms.Label();
            this.Label2 = new System.Windows.Forms.Label();
            this.check_grayscale = new System.Windows.Forms.CheckBox();
            this.SliderValue = new System.Windows.Forms.Label();
            this.check_DeInterlace = new System.Windows.Forms.CheckBox();
            this.drp_videoFramerate = new System.Windows.Forms.ComboBox();
            this.check_2PassEncode = new System.Windows.Forms.CheckBox();
            this.slider_videoQuality = new System.Windows.Forms.TrackBar();
            this.text_filesize = new System.Windows.Forms.TextBox();
            this.Label46 = new System.Windows.Forms.Label();
            this.Label40 = new System.Windows.Forms.Label();
            this.text_bitrate = new System.Windows.Forms.TextBox();
            this.Label42 = new System.Windows.Forms.Label();
            this.TabPage2 = new System.Windows.Forms.TabPage();
            this.Label29 = new System.Windows.Forms.Label();
            this.drp_audioMixDown = new System.Windows.Forms.ComboBox();
            this.drp_audioChannels = new System.Windows.Forms.ComboBox();
            this.drp_audioBitrate = new System.Windows.Forms.ComboBox();
            this.Label14 = new System.Windows.Forms.Label();
            this.Label5 = new System.Windows.Forms.Label();
            this.Label35 = new System.Windows.Forms.Label();
            this.Label16 = new System.Windows.Forms.Label();
            this.Label32 = new System.Windows.Forms.Label();
            this.Label18 = new System.Windows.Forms.Label();
            this.drp_audioSampleRate = new System.Windows.Forms.ComboBox();
            this.h264Tab = new System.Windows.Forms.TabPage();
            this.Label43 = new System.Windows.Forms.Label();
            this.label_h264 = new System.Windows.Forms.LinkLabel();
            this.Label95 = new System.Windows.Forms.Label();
            this.btn_h264Clear = new System.Windows.Forms.Button();
            this.Label90 = new System.Windows.Forms.Label();
            this.rtf_h264advanced = new System.Windows.Forms.RichTextBox();
            this.Label92 = new System.Windows.Forms.Label();
            this.TabPage6 = new System.Windows.Forms.TabPage();
            this.Label7 = new System.Windows.Forms.Label();
            this.Label39 = new System.Windows.Forms.Label();
            this.btn_ClearQuery = new System.Windows.Forms.Button();
            this.GenerateQuery = new System.Windows.Forms.Button();
            this.QueryEditorText = new System.Windows.Forms.RichTextBox();
            this.lbl_update = new System.Windows.Forms.Label();
            this.btn_queue = new System.Windows.Forms.Button();
            this.btn_encode = new System.Windows.Forms.Button();
            this.Version = new System.Windows.Forms.Label();
            this.lbl_chptWarn = new System.Windows.Forms.Label();
            Label38 = new System.Windows.Forms.Label();
            this.frmMainMenu.SuspendLayout();
            this.GroupBox1.SuspendLayout();
            this.GroupBox4.SuspendLayout();
            this.advancedOptions.SuspendLayout();
            this.TabPage1.SuspendLayout();
            this.TabPage3.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).BeginInit();
            this.TabPage2.SuspendLayout();
            this.h264Tab.SuspendLayout();
            this.TabPage6.SuspendLayout();
            this.SuspendLayout();
            // 
            // Label38
            // 
            Label38.AutoSize = true;
            Label38.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            Label38.Location = new System.Drawing.Point(13, 67);
            Label38.Name = "Label38";
            Label38.Size = new System.Drawing.Size(103, 13);
            Label38.TabIndex = 30;
            Label38.Text = "Target Size (MB)";
            // 
            // DVD_Save
            // 
            this.DVD_Save.DefaultExt = "mp4";
            this.DVD_Save.Filter = "mp4|*.mp4 |m4v|*.m4v |avi|*.avi |ogm|*.ogm |mkv|*.mkv";
            // 
            // File_Save
            // 
            this.File_Save.DefaultExt = "hb";
            this.File_Save.Filter = "hb|*.hb";
            // 
            // drop_chapterFinish
            // 
            this.drop_chapterFinish.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_chapterFinish.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_chapterFinish.FormattingEnabled = true;
            this.drop_chapterFinish.Location = new System.Drawing.Point(212, 81);
            this.drop_chapterFinish.Name = "drop_chapterFinish";
            this.drop_chapterFinish.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterFinish.TabIndex = 41;
            this.drop_chapterFinish.Text = "Auto";
            this.ToolTip.SetToolTip(this.drop_chapterFinish, "Encode chapters to this number");
            this.drop_chapterFinish.SelectedIndexChanged += new System.EventHandler(this.drop_chapterFinish_SelectedIndexChanged);
            // 
            // drop_chapterStart
            // 
            this.drop_chapterStart.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drop_chapterStart.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drop_chapterStart.FormattingEnabled = true;
            this.drop_chapterStart.Location = new System.Drawing.Point(99, 81);
            this.drop_chapterStart.Name = "drop_chapterStart";
            this.drop_chapterStart.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterStart.TabIndex = 40;
            this.drop_chapterStart.Text = "Auto";
            this.ToolTip.SetToolTip(this.drop_chapterStart, "Encode chatpers from this number.");
            this.drop_chapterStart.SelectedIndexChanged += new System.EventHandler(this.drop_chapterStart_SelectedIndexChanged);
            // 
            // drp_dvdtitle
            // 
            this.drp_dvdtitle.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_dvdtitle.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_dvdtitle.FormattingEnabled = true;
            this.drp_dvdtitle.Items.AddRange(new object[] {
            "Automatic"});
            this.drp_dvdtitle.Location = new System.Drawing.Point(99, 50);
            this.drp_dvdtitle.Name = "drp_dvdtitle";
            this.drp_dvdtitle.Size = new System.Drawing.Size(119, 21);
            this.drp_dvdtitle.TabIndex = 39;
            this.drp_dvdtitle.Text = "Automatic";
            this.ToolTip.SetToolTip(this.drp_dvdtitle, "The title number you wish to encode.");
            this.drp_dvdtitle.SelectedIndexChanged += new System.EventHandler(this.drp_dvdtitle_SelectedIndexChanged);
            this.drp_dvdtitle.Click += new System.EventHandler(this.drp_dvdtitle_Click);
            // 
            // RadioISO
            // 
            this.RadioISO.AutoSize = true;
            this.RadioISO.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.RadioISO.Location = new System.Drawing.Point(358, 34);
            this.RadioISO.Name = "RadioISO";
            this.RadioISO.Size = new System.Drawing.Size(44, 17);
            this.RadioISO.TabIndex = 19;
            this.RadioISO.Text = "File";
            this.ToolTip.SetToolTip(this.RadioISO, "ISO, TS, MPG");
            this.RadioISO.UseVisualStyleBackColor = true;
            // 
            // text_source
            // 
            this.text_source.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_source.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_source.Location = new System.Drawing.Point(99, 22);
            this.text_source.Name = "text_source";
            this.text_source.Size = new System.Drawing.Size(253, 21);
            this.text_source.TabIndex = 1;
            this.text_source.Text = "Click \'Browse\' to continue";
            this.ToolTip.SetToolTip(this.text_source, "The input source location.");
            // 
            // text_destination
            // 
            this.text_destination.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_destination.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_destination.Location = new System.Drawing.Point(99, 21);
            this.text_destination.Name = "text_destination";
            this.text_destination.Size = new System.Drawing.Size(262, 21);
            this.text_destination.TabIndex = 4;
            this.ToolTip.SetToolTip(this.text_destination, "Where you wish to save your output file.");
            // 
            // drp_subtitle
            // 
            this.drp_subtitle.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_subtitle.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_subtitle.FormattingEnabled = true;
            this.drp_subtitle.Location = new System.Drawing.Point(381, 36);
            this.drp_subtitle.Name = "drp_subtitle";
            this.drp_subtitle.Size = new System.Drawing.Size(213, 21);
            this.drp_subtitle.TabIndex = 42;
            this.drp_subtitle.Text = "None";
            this.ToolTip.SetToolTip(this.drp_subtitle, "Select the subtitle language you require from this dropdown.");
            // 
            // File_Open
            // 
            this.File_Open.DefaultExt = "hb";
            this.File_Open.Filter = "hb|*.hb";
            // 
            // ISO_Open
            // 
            this.ISO_Open.DefaultExt = "ISO";
            this.ISO_Open.Filter = "All Supported Files|*.iso;*.mpg;*.mpeg;*.vob";
            // 
            // FileToolStripMenuItem
            // 
            this.FileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_open,
            this.mnu_save,
            this.ToolStripSeparator1,
            this.mnu_update,
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
            this.mnu_open.Size = new System.Drawing.Size(184, 22);
            this.mnu_open.Text = "&Open Profile";
            this.mnu_open.Click += new System.EventHandler(this.mnu_open_Click);
            // 
            // mnu_save
            // 
            this.mnu_save.Image = ((System.Drawing.Image)(resources.GetObject("mnu_save.Image")));
            this.mnu_save.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.mnu_save.Name = "mnu_save";
            this.mnu_save.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            this.mnu_save.Size = new System.Drawing.Size(184, 22);
            this.mnu_save.Text = "&Save Profile";
            this.mnu_save.Click += new System.EventHandler(this.mnu_save_Click);
            // 
            // ToolStripSeparator1
            // 
            this.ToolStripSeparator1.Name = "ToolStripSeparator1";
            this.ToolStripSeparator1.Size = new System.Drawing.Size(181, 6);
            // 
            // mnu_update
            // 
            this.mnu_update.Name = "mnu_update";
            this.mnu_update.Size = new System.Drawing.Size(184, 22);
            this.mnu_update.Text = "Check for Updates";
            this.mnu_update.Click += new System.EventHandler(this.mnu_update_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(181, 6);
            // 
            // mnu_exit
            // 
            this.mnu_exit.Name = "mnu_exit";
            this.mnu_exit.Size = new System.Drawing.Size(184, 22);
            this.mnu_exit.Text = "E&xit";
            this.mnu_exit.Click += new System.EventHandler(this.mnu_exit_Click);
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
            this.mnu_encode.Name = "mnu_encode";
            this.mnu_encode.Size = new System.Drawing.Size(155, 22);
            this.mnu_encode.Text = "Encode Queue";
            this.mnu_encode.Click += new System.EventHandler(this.mnu_encode_Click);
            // 
            // mnu_viewDVDdata
            // 
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
            this.mnu_options.Name = "mnu_options";
            this.mnu_options.Size = new System.Drawing.Size(155, 22);
            this.mnu_options.Text = "Options";
            this.mnu_options.Click += new System.EventHandler(this.mnu_options_Click);
            // 
            // PresetsToolStripMenuItem
            // 
            this.PresetsToolStripMenuItem.BackColor = System.Drawing.SystemColors.Control;
            this.PresetsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_preset_ipod133,
            this.mnu_preset_ipod178,
            this.mnu_preset_ipod235,
            this.mnu_appleTv,
            this.mnu_presetPS3,
            this.ToolStripSeparator4,
            this.mnu_ProgramDefaultOptions});
            this.PresetsToolStripMenuItem.Name = "PresetsToolStripMenuItem";
            this.PresetsToolStripMenuItem.Size = new System.Drawing.Size(55, 20);
            this.PresetsToolStripMenuItem.Text = "&Presets";
            // 
            // mnu_preset_ipod133
            // 
            this.mnu_preset_ipod133.Name = "mnu_preset_ipod133";
            this.mnu_preset_ipod133.Size = new System.Drawing.Size(276, 22);
            this.mnu_preset_ipod133.Text = "iPod (1.33)";
            this.mnu_preset_ipod133.Click += new System.EventHandler(this.mnu_preset_ipod133_Click);
            // 
            // mnu_preset_ipod178
            // 
            this.mnu_preset_ipod178.Name = "mnu_preset_ipod178";
            this.mnu_preset_ipod178.Size = new System.Drawing.Size(276, 22);
            this.mnu_preset_ipod178.Text = "iPod (1.78)";
            this.mnu_preset_ipod178.Click += new System.EventHandler(this.mnu_preset_ipod178_Click);
            // 
            // mnu_preset_ipod235
            // 
            this.mnu_preset_ipod235.Name = "mnu_preset_ipod235";
            this.mnu_preset_ipod235.Size = new System.Drawing.Size(276, 22);
            this.mnu_preset_ipod235.Text = "iPod (2.35)";
            this.mnu_preset_ipod235.Click += new System.EventHandler(this.mnu_preset_ipod235_Click);
            // 
            // mnu_appleTv
            // 
            this.mnu_appleTv.Name = "mnu_appleTv";
            this.mnu_appleTv.Size = new System.Drawing.Size(276, 22);
            this.mnu_appleTv.Text = "Apple TV";
            this.mnu_appleTv.Click += new System.EventHandler(this.mnu_appleTv_Click);
            // 
            // mnu_presetPS3
            // 
            this.mnu_presetPS3.Name = "mnu_presetPS3";
            this.mnu_presetPS3.Size = new System.Drawing.Size(276, 22);
            this.mnu_presetPS3.Text = "PS3";
            this.mnu_presetPS3.Click += new System.EventHandler(this.mnu_presetPS3_Click);
            // 
            // ToolStripSeparator4
            // 
            this.ToolStripSeparator4.Name = "ToolStripSeparator4";
            this.ToolStripSeparator4.Size = new System.Drawing.Size(273, 6);
            // 
            // mnu_ProgramDefaultOptions
            // 
            this.mnu_ProgramDefaultOptions.Name = "mnu_ProgramDefaultOptions";
            this.mnu_ProgramDefaultOptions.Size = new System.Drawing.Size(276, 22);
            this.mnu_ProgramDefaultOptions.Text = "Set current options as program defaults";
            this.mnu_ProgramDefaultOptions.Click += new System.EventHandler(this.mnu_ProgramDefaultOptions_Click);
            // 
            // HelpToolStripMenuItem
            // 
            this.HelpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.OnlineDocumentationToolStripMenuItem,
            this.WebsiteToolStripMenuItem,
            this.ToolStripSeparator3,
            this.mnu_about});
            this.HelpToolStripMenuItem.Name = "HelpToolStripMenuItem";
            this.HelpToolStripMenuItem.Size = new System.Drawing.Size(40, 20);
            this.HelpToolStripMenuItem.Text = "&Help";
            // 
            // OnlineDocumentationToolStripMenuItem
            // 
            this.OnlineDocumentationToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_wiki,
            this.mnu_onlineDocs,
            this.mnu_faq});
            this.OnlineDocumentationToolStripMenuItem.Name = "OnlineDocumentationToolStripMenuItem";
            this.OnlineDocumentationToolStripMenuItem.Size = new System.Drawing.Size(190, 22);
            this.OnlineDocumentationToolStripMenuItem.Text = "Online Documentation";
            // 
            // mnu_wiki
            // 
            this.mnu_wiki.Name = "mnu_wiki";
            this.mnu_wiki.Size = new System.Drawing.Size(157, 22);
            this.mnu_wiki.Text = "Wiki";
            this.mnu_wiki.Click += new System.EventHandler(this.mnu_wiki_Click);
            // 
            // mnu_onlineDocs
            // 
            this.mnu_onlineDocs.Name = "mnu_onlineDocs";
            this.mnu_onlineDocs.Size = new System.Drawing.Size(157, 22);
            this.mnu_onlineDocs.Text = "Documentation";
            this.mnu_onlineDocs.Click += new System.EventHandler(this.mnu_onlineDocs_Click);
            // 
            // mnu_faq
            // 
            this.mnu_faq.Name = "mnu_faq";
            this.mnu_faq.Size = new System.Drawing.Size(157, 22);
            this.mnu_faq.Text = "FAQ";
            this.mnu_faq.Click += new System.EventHandler(this.mnu_faq_Click);
            // 
            // WebsiteToolStripMenuItem
            // 
            this.WebsiteToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_homepage,
            this.mnu_forum});
            this.WebsiteToolStripMenuItem.Name = "WebsiteToolStripMenuItem";
            this.WebsiteToolStripMenuItem.Size = new System.Drawing.Size(190, 22);
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
            this.ToolStripSeparator3.Size = new System.Drawing.Size(187, 6);
            // 
            // mnu_about
            // 
            this.mnu_about.Name = "mnu_about";
            this.mnu_about.Size = new System.Drawing.Size(190, 22);
            this.mnu_about.Text = "About...";
            this.mnu_about.Click += new System.EventHandler(this.mnu_about_Click);
            // 
            // frmMainMenu
            // 
            this.frmMainMenu.BackColor = System.Drawing.SystemColors.Control;
            this.frmMainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileToolStripMenuItem,
            this.ToolsToolStripMenuItem,
            this.PresetsToolStripMenuItem,
            this.HelpToolStripMenuItem});
            this.frmMainMenu.Location = new System.Drawing.Point(0, 0);
            this.frmMainMenu.Name = "frmMainMenu";
            this.frmMainMenu.Size = new System.Drawing.Size(675, 24);
            this.frmMainMenu.TabIndex = 1;
            this.frmMainMenu.Text = "MenuStrip1";
            // 
            // GroupBox1
            // 
            this.GroupBox1.BackColor = System.Drawing.SystemColors.Control;
            this.GroupBox1.Controls.Add(this.lbl_chptWarn);
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
            this.GroupBox1.Location = new System.Drawing.Point(15, 35);
            this.GroupBox1.Name = "GroupBox1";
            this.GroupBox1.Size = new System.Drawing.Size(647, 116);
            this.GroupBox1.TabIndex = 408;
            this.GroupBox1.TabStop = false;
            this.GroupBox1.Text = "Source";
            // 
            // Label13
            // 
            this.Label13.AutoSize = true;
            this.Label13.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label13.Location = new System.Drawing.Point(178, 85);
            this.Label13.Name = "Label13";
            this.Label13.Size = new System.Drawing.Size(21, 13);
            this.Label13.TabIndex = 42;
            this.Label13.Text = "To";
            // 
            // RadioDVD
            // 
            this.RadioDVD.AutoSize = true;
            this.RadioDVD.Checked = true;
            this.RadioDVD.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.RadioDVD.Location = new System.Drawing.Point(358, 18);
            this.RadioDVD.Name = "RadioDVD";
            this.RadioDVD.Size = new System.Drawing.Size(51, 17);
            this.RadioDVD.TabIndex = 20;
            this.RadioDVD.TabStop = true;
            this.RadioDVD.Text = "DVD";
            this.RadioDVD.UseVisualStyleBackColor = true;
            // 
            // btn_Browse
            // 
            this.btn_Browse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_Browse.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_Browse.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_Browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Browse.Location = new System.Drawing.Point(415, 22);
            this.btn_Browse.Name = "btn_Browse";
            this.btn_Browse.Size = new System.Drawing.Size(78, 22);
            this.btn_Browse.TabIndex = 2;
            this.btn_Browse.Text = "Browse";
            this.btn_Browse.UseVisualStyleBackColor = true;
            this.btn_Browse.Click += new System.EventHandler(this.btn_Browse_Click);
            // 
            // Label17
            // 
            this.Label17.AutoSize = true;
            this.Label17.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label17.ForeColor = System.Drawing.Color.Black;
            this.Label17.Location = new System.Drawing.Point(12, 25);
            this.Label17.Name = "Label17";
            this.Label17.Size = new System.Drawing.Size(52, 13);
            this.Label17.TabIndex = 6;
            this.Label17.Text = "Source:";
            // 
            // Label9
            // 
            this.Label9.AutoSize = true;
            this.Label9.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label9.ForeColor = System.Drawing.Color.Black;
            this.Label9.Location = new System.Drawing.Point(12, 84);
            this.Label9.Name = "Label9";
            this.Label9.Size = new System.Drawing.Size(64, 13);
            this.Label9.TabIndex = 12;
            this.Label9.Text = "Chapters:";
            // 
            // Label10
            // 
            this.Label10.AutoSize = true;
            this.Label10.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label10.ForeColor = System.Drawing.Color.Black;
            this.Label10.Location = new System.Drawing.Point(12, 56);
            this.Label10.Name = "Label10";
            this.Label10.Size = new System.Drawing.Size(36, 13);
            this.Label10.TabIndex = 11;
            this.Label10.Text = "Title:";
            // 
            // GroupBox4
            // 
            this.GroupBox4.BackColor = System.Drawing.SystemColors.Control;
            this.GroupBox4.Controls.Add(this.Label56);
            this.GroupBox4.Controls.Add(this.lbl_Aspect);
            this.GroupBox4.Controls.Add(this.Label91);
            this.GroupBox4.Controls.Add(this.text_height);
            this.GroupBox4.Controls.Add(this.Label55);
            this.GroupBox4.Controls.Add(this.text_width);
            this.GroupBox4.Controls.Add(this.btn_destBrowse);
            this.GroupBox4.Controls.Add(this.Label3);
            this.GroupBox4.Controls.Add(this.drp_videoEncoder);
            this.GroupBox4.Controls.Add(this.Label47);
            this.GroupBox4.Controls.Add(this.text_destination);
            this.GroupBox4.Controls.Add(this.drp_audioCodec);
            this.GroupBox4.Controls.Add(this.Label12);
            this.GroupBox4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GroupBox4.ForeColor = System.Drawing.Color.Black;
            this.GroupBox4.Location = new System.Drawing.Point(15, 157);
            this.GroupBox4.Name = "GroupBox4";
            this.GroupBox4.Size = new System.Drawing.Size(647, 126);
            this.GroupBox4.TabIndex = 409;
            this.GroupBox4.TabStop = false;
            this.GroupBox4.Text = "Destination";
            // 
            // Label56
            // 
            this.Label56.AutoSize = true;
            this.Label56.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label56.ForeColor = System.Drawing.Color.Black;
            this.Label56.Location = new System.Drawing.Point(170, 90);
            this.Label56.Name = "Label56";
            this.Label56.Size = new System.Drawing.Size(15, 13);
            this.Label56.TabIndex = 38;
            this.Label56.Text = "x";
            // 
            // lbl_Aspect
            // 
            this.lbl_Aspect.AutoSize = true;
            this.lbl_Aspect.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_Aspect.Location = new System.Drawing.Point(369, 91);
            this.lbl_Aspect.Name = "lbl_Aspect";
            this.lbl_Aspect.Size = new System.Drawing.Size(72, 12);
            this.lbl_Aspect.TabIndex = 41;
            this.lbl_Aspect.Text = "Select a Title";
            // 
            // Label91
            // 
            this.Label91.AutoSize = true;
            this.Label91.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label91.Location = new System.Drawing.Point(265, 91);
            this.Label91.Name = "Label91";
            this.Label91.Size = new System.Drawing.Size(87, 13);
            this.Label91.TabIndex = 40;
            this.Label91.Text = "Aspect Ratio: ";
            // 
            // text_height
            // 
            this.text_height.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_height.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_height.ForeColor = System.Drawing.SystemColors.InfoText;
            this.text_height.Location = new System.Drawing.Point(191, 87);
            this.text_height.Name = "text_height";
            this.text_height.Size = new System.Drawing.Size(64, 21);
            this.text_height.TabIndex = 8;
            this.text_height.TextChanged += new System.EventHandler(this.text_height_TextChanged);
            // 
            // Label55
            // 
            this.Label55.AutoSize = true;
            this.Label55.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label55.ForeColor = System.Drawing.Color.Black;
            this.Label55.Location = new System.Drawing.Point(12, 91);
            this.Label55.Name = "Label55";
            this.Label55.Size = new System.Drawing.Size(85, 13);
            this.Label55.TabIndex = 29;
            this.Label55.Text = "Width/Height:";
            // 
            // text_width
            // 
            this.text_width.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_width.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_width.Location = new System.Drawing.Point(99, 87);
            this.text_width.Name = "text_width";
            this.text_width.Size = new System.Drawing.Size(64, 21);
            this.text_width.TabIndex = 7;
            this.text_width.TextChanged += new System.EventHandler(this.text_width_TextChanged);
            // 
            // btn_destBrowse
            // 
            this.btn_destBrowse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_destBrowse.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_destBrowse.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_destBrowse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_destBrowse.Location = new System.Drawing.Point(370, 21);
            this.btn_destBrowse.Name = "btn_destBrowse";
            this.btn_destBrowse.Size = new System.Drawing.Size(83, 22);
            this.btn_destBrowse.TabIndex = 4;
            this.btn_destBrowse.Text = "Browse";
            this.btn_destBrowse.UseVisualStyleBackColor = true;
            this.btn_destBrowse.Click += new System.EventHandler(this.btn_destBrowse_Click);
            // 
            // Label3
            // 
            this.Label3.AutoSize = true;
            this.Label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label3.ForeColor = System.Drawing.Color.Black;
            this.Label3.Location = new System.Drawing.Point(12, 25);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(80, 13);
            this.Label3.TabIndex = 3;
            this.Label3.Text = "Destination: ";
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
            "H.264 (iPod)",
            "H.264 Baseline 1.3"});
            this.drp_videoEncoder.Location = new System.Drawing.Point(99, 54);
            this.drp_videoEncoder.Name = "drp_videoEncoder";
            this.drp_videoEncoder.Size = new System.Drawing.Size(156, 21);
            this.drp_videoEncoder.TabIndex = 5;
            this.drp_videoEncoder.Text = "H.264";
            this.drp_videoEncoder.SelectedIndexChanged += new System.EventHandler(this.drp_videoEncoder_SelectedIndexChanged);
            // 
            // Label47
            // 
            this.Label47.AutoSize = true;
            this.Label47.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label47.ForeColor = System.Drawing.Color.Black;
            this.Label47.Location = new System.Drawing.Point(12, 57);
            this.Label47.Name = "Label47";
            this.Label47.Size = new System.Drawing.Size(62, 13);
            this.Label47.TabIndex = 12;
            this.Label47.Text = "Encoder: ";
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
            this.drp_audioCodec.Location = new System.Drawing.Point(371, 53);
            this.drp_audioCodec.Name = "drp_audioCodec";
            this.drp_audioCodec.Size = new System.Drawing.Size(111, 21);
            this.drp_audioCodec.TabIndex = 6;
            this.drp_audioCodec.Text = "AAC";
            this.drp_audioCodec.SelectedIndexChanged += new System.EventHandler(this.drp_audioCodec_SelectedIndexChanged);
            // 
            // Label12
            // 
            this.Label12.AutoSize = true;
            this.Label12.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label12.ForeColor = System.Drawing.Color.Black;
            this.Label12.Location = new System.Drawing.Point(265, 57);
            this.Label12.Name = "Label12";
            this.Label12.Size = new System.Drawing.Size(94, 13);
            this.Label12.TabIndex = 20;
            this.Label12.Text = "Audio Encoder:";
            // 
            // advancedOptions
            // 
            this.advancedOptions.Controls.Add(this.TabPage1);
            this.advancedOptions.Controls.Add(this.TabPage3);
            this.advancedOptions.Controls.Add(this.TabPage2);
            this.advancedOptions.Controls.Add(this.h264Tab);
            this.advancedOptions.Controls.Add(this.TabPage6);
            this.advancedOptions.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.advancedOptions.Location = new System.Drawing.Point(15, 289);
            this.advancedOptions.Name = "advancedOptions";
            this.advancedOptions.SelectedIndex = 0;
            this.advancedOptions.Size = new System.Drawing.Size(647, 294);
            this.advancedOptions.TabIndex = 411;
            this.advancedOptions.TabStop = false;
            // 
            // TabPage1
            // 
            this.TabPage1.BackColor = System.Drawing.SystemColors.Control;
            this.TabPage1.Controls.Add(this.drp_subtitle);
            this.TabPage1.Controls.Add(this.Label19);
            this.TabPage1.Controls.Add(this.lbl_RecomendedCrop);
            this.TabPage1.Controls.Add(this.Label8);
            this.TabPage1.Controls.Add(this.Label1);
            this.TabPage1.Controls.Add(this.Label53);
            this.TabPage1.Controls.Add(this.Label21);
            this.TabPage1.Controls.Add(this.Label20);
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
            this.TabPage1.Size = new System.Drawing.Size(639, 268);
            this.TabPage1.TabIndex = 0;
            this.TabPage1.Text = "Picture Settings";
            // 
            // Label19
            // 
            this.Label19.AutoSize = true;
            this.Label19.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label19.Location = new System.Drawing.Point(311, 13);
            this.Label19.Name = "Label19";
            this.Label19.Size = new System.Drawing.Size(64, 13);
            this.Label19.TabIndex = 39;
            this.Label19.Text = "Subtitles";
            // 
            // lbl_RecomendedCrop
            // 
            this.lbl_RecomendedCrop.AutoSize = true;
            this.lbl_RecomendedCrop.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_RecomendedCrop.Location = new System.Drawing.Point(119, 72);
            this.lbl_RecomendedCrop.Name = "lbl_RecomendedCrop";
            this.lbl_RecomendedCrop.Size = new System.Drawing.Size(72, 12);
            this.lbl_RecomendedCrop.TabIndex = 38;
            this.lbl_RecomendedCrop.Text = "Select a Title";
            // 
            // Label8
            // 
            this.Label8.AutoSize = true;
            this.Label8.Location = new System.Drawing.Point(13, 71);
            this.Label8.Name = "Label8";
            this.Label8.Size = new System.Drawing.Size(70, 13);
            this.Label8.TabIndex = 37;
            this.Label8.Text = "Auto Crop:";
            // 
            // Label1
            // 
            this.Label1.AutoSize = true;
            this.Label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label1.Location = new System.Drawing.Point(13, 13);
            this.Label1.Name = "Label1";
            this.Label1.Size = new System.Drawing.Size(65, 13);
            this.Label1.TabIndex = 36;
            this.Label1.Text = "Cropping";
            // 
            // Label53
            // 
            this.Label53.AutoSize = true;
            this.Label53.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label53.Location = new System.Drawing.Point(134, 196);
            this.Label53.Name = "Label53";
            this.Label53.Size = new System.Drawing.Size(48, 13);
            this.Label53.TabIndex = 32;
            this.Label53.Text = "Bottom";
            // 
            // Label21
            // 
            this.Label21.AutoSize = true;
            this.Label21.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label21.Location = new System.Drawing.Point(378, 63);
            this.Label21.Name = "Label21";
            this.Label21.Size = new System.Drawing.Size(224, 26);
            this.Label21.TabIndex = 34;
            this.Label21.Text = "Please note that subtitles will be hard \r\ncoded into the video.\r\n";
            // 
            // Label20
            // 
            this.Label20.AutoSize = true;
            this.Label20.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label20.Location = new System.Drawing.Point(314, 39);
            this.Label20.Name = "Label20";
            this.Label20.Size = new System.Drawing.Size(61, 13);
            this.Label20.TabIndex = 33;
            this.Label20.Text = "Subtitles:";
            // 
            // Label52
            // 
            this.Label52.AutoSize = true;
            this.Label52.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label52.Location = new System.Drawing.Point(138, 105);
            this.Label52.Name = "Label52";
            this.Label52.Size = new System.Drawing.Size(28, 13);
            this.Label52.TabIndex = 31;
            this.Label52.Text = "Top";
            // 
            // Label51
            // 
            this.Label51.AutoSize = true;
            this.Label51.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label51.Location = new System.Drawing.Point(243, 148);
            this.Label51.Name = "Label51";
            this.Label51.Size = new System.Drawing.Size(36, 13);
            this.Label51.TabIndex = 30;
            this.Label51.Text = "Right";
            // 
            // Label50
            // 
            this.Label50.AutoSize = true;
            this.Label50.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label50.Location = new System.Drawing.Point(13, 41);
            this.Label50.Name = "Label50";
            this.Label50.Size = new System.Drawing.Size(88, 13);
            this.Label50.TabIndex = 17;
            this.Label50.Text = "Select Option:";
            // 
            // Label15
            // 
            this.Label15.AutoSize = true;
            this.Label15.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label15.Location = new System.Drawing.Point(39, 148);
            this.Label15.Name = "Label15";
            this.Label15.Size = new System.Drawing.Size(28, 13);
            this.Label15.TabIndex = 29;
            this.Label15.Text = "Left";
            // 
            // text_top
            // 
            this.text_top.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_top.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_top.Location = new System.Drawing.Point(131, 120);
            this.text_top.Name = "text_top";
            this.text_top.Size = new System.Drawing.Size(51, 21);
            this.text_top.TabIndex = 10;
            this.text_top.Text = "0";
            // 
            // text_bottom
            // 
            this.text_bottom.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_bottom.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_bottom.Location = new System.Drawing.Point(131, 172);
            this.text_bottom.Name = "text_bottom";
            this.text_bottom.Size = new System.Drawing.Size(51, 21);
            this.text_bottom.TabIndex = 12;
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
            this.drp_crop.Location = new System.Drawing.Point(121, 36);
            this.drp_crop.Name = "drp_crop";
            this.drp_crop.Size = new System.Drawing.Size(123, 21);
            this.drp_crop.TabIndex = 9;
            this.drp_crop.Text = "No Crop";
            this.drp_crop.SelectedIndexChanged += new System.EventHandler(this.drp_crop_SelectedIndexChanged);
            // 
            // text_right
            // 
            this.text_right.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_right.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_right.Location = new System.Drawing.Point(190, 145);
            this.text_right.Name = "text_right";
            this.text_right.Size = new System.Drawing.Size(51, 21);
            this.text_right.TabIndex = 13;
            this.text_right.Text = "0";
            // 
            // text_left
            // 
            this.text_left.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_left.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_left.Location = new System.Drawing.Point(72, 145);
            this.text_left.Name = "text_left";
            this.text_left.Size = new System.Drawing.Size(51, 21);
            this.text_left.TabIndex = 11;
            this.text_left.Text = "0";
            // 
            // TabPage3
            // 
            this.TabPage3.BackColor = System.Drawing.SystemColors.Control;
            this.TabPage3.Controls.Add(this.Label41);
            this.TabPage3.Controls.Add(this.Label37);
            this.TabPage3.Controls.Add(this.check_largeFile);
            this.TabPage3.Controls.Add(this.check_turbo);
            this.TabPage3.Controls.Add(this.Label36);
            this.TabPage3.Controls.Add(this.Check_ChapterMarkers);
            this.TabPage3.Controls.Add(this.Label28);
            this.TabPage3.Controls.Add(this.Label27);
            this.TabPage3.Controls.Add(this.Label4);
            this.TabPage3.Controls.Add(this.CheckCRF);
            this.TabPage3.Controls.Add(this.CheckPixelRatio);
            this.TabPage3.Controls.Add(this.Label23);
            this.TabPage3.Controls.Add(this.Label22);
            this.TabPage3.Controls.Add(this.Label2);
            this.TabPage3.Controls.Add(this.check_grayscale);
            this.TabPage3.Controls.Add(this.SliderValue);
            this.TabPage3.Controls.Add(this.check_DeInterlace);
            this.TabPage3.Controls.Add(this.drp_videoFramerate);
            this.TabPage3.Controls.Add(this.check_2PassEncode);
            this.TabPage3.Controls.Add(this.slider_videoQuality);
            this.TabPage3.Controls.Add(this.text_filesize);
            this.TabPage3.Controls.Add(Label38);
            this.TabPage3.Controls.Add(this.Label46);
            this.TabPage3.Controls.Add(this.Label40);
            this.TabPage3.Controls.Add(this.text_bitrate);
            this.TabPage3.Controls.Add(this.Label42);
            this.TabPage3.Location = new System.Drawing.Point(4, 22);
            this.TabPage3.Name = "TabPage3";
            this.TabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage3.Size = new System.Drawing.Size(639, 268);
            this.TabPage3.TabIndex = 2;
            this.TabPage3.Text = "Video Settings";
            // 
            // Label41
            // 
            this.Label41.AutoSize = true;
            this.Label41.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label41.Location = new System.Drawing.Point(494, 63);
            this.Label41.Name = "Label41";
            this.Label41.Size = new System.Drawing.Size(121, 12);
            this.Label41.TabIndex = 53;
            this.Label41.Text = "(H.264 encoder\'s only)";
            // 
            // Label37
            // 
            this.Label37.AutoSize = true;
            this.Label37.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label37.Location = new System.Drawing.Point(382, 194);
            this.Label37.Name = "Label37";
            this.Label37.Size = new System.Drawing.Size(228, 12);
            this.Label37.TabIndex = 52;
            this.Label37.Text = " Note: Breaks iPod, @TV, PS3 compatibility.";
            // 
            // check_largeFile
            // 
            this.check_largeFile.AutoSize = true;
            this.check_largeFile.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_largeFile.Location = new System.Drawing.Point(367, 176);
            this.check_largeFile.Name = "check_largeFile";
            this.check_largeFile.Size = new System.Drawing.Size(172, 17);
            this.check_largeFile.TabIndex = 51;
            this.check_largeFile.Text = "Larger mp4 Files (> 4GB)";
            this.check_largeFile.UseVisualStyleBackColor = true;
            this.check_largeFile.Click += new System.EventHandler(this.check_largeFile_CheckedChanged);
            // 
            // check_turbo
            // 
            this.check_turbo.AutoSize = true;
            this.check_turbo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_turbo.Location = new System.Drawing.Point(387, 61);
            this.check_turbo.Name = "check_turbo";
            this.check_turbo.Size = new System.Drawing.Size(110, 17);
            this.check_turbo.TabIndex = 50;
            this.check_turbo.Text = "Turbo 1st Pass";
            this.check_turbo.UseVisualStyleBackColor = true;
            this.check_turbo.Click += new System.EventHandler(this.check_turbo_CheckedChanged);
            // 
            // Label36
            // 
            this.Label36.AutoSize = true;
            this.Label36.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label36.Location = new System.Drawing.Point(486, 155);
            this.Label36.Name = "Label36";
            this.Label36.Size = new System.Drawing.Size(109, 12);
            this.Label36.TabIndex = 49;
            this.Label36.Text = "(m4v container only)";
            // 
            // Check_ChapterMarkers
            // 
            this.Check_ChapterMarkers.AutoSize = true;
            this.Check_ChapterMarkers.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Check_ChapterMarkers.Location = new System.Drawing.Point(368, 153);
            this.Check_ChapterMarkers.Name = "Check_ChapterMarkers";
            this.Check_ChapterMarkers.Size = new System.Drawing.Size(122, 17);
            this.Check_ChapterMarkers.TabIndex = 48;
            this.Check_ChapterMarkers.Text = "Chapter Markers";
            this.Check_ChapterMarkers.UseVisualStyleBackColor = true;
            this.Check_ChapterMarkers.CheckedChanged += new System.EventHandler(this.Check_ChapterMarkers_CheckedChanged);
            // 
            // Label28
            // 
            this.Label28.AutoSize = true;
            this.Label28.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label28.Location = new System.Drawing.Point(108, 237);
            this.Label28.Name = "Label28";
            this.Label28.Size = new System.Drawing.Size(121, 12);
            this.Label28.TabIndex = 47;
            this.Label28.Text = "(H.264 encoder\'s only)";
            // 
            // Label27
            // 
            this.Label27.AutoSize = true;
            this.Label27.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label27.Location = new System.Drawing.Point(108, 219);
            this.Label27.Name = "Label27";
            this.Label27.Size = new System.Drawing.Size(205, 12);
            this.Label27.TabIndex = 46;
            this.Label27.Text = "(To be used with \"Video Quality\" Slider)";
            // 
            // Label4
            // 
            this.Label4.AutoSize = true;
            this.Label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label4.Location = new System.Drawing.Point(13, 193);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(147, 13);
            this.Label4.TabIndex = 45;
            this.Label4.Text = "Constant Rate Factor ";
            // 
            // CheckCRF
            // 
            this.CheckCRF.AutoSize = true;
            this.CheckCRF.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.CheckCRF.Location = new System.Drawing.Point(16, 217);
            this.CheckCRF.Name = "CheckCRF";
            this.CheckCRF.Size = new System.Drawing.Size(91, 17);
            this.CheckCRF.TabIndex = 44;
            this.CheckCRF.Text = "Enable CRF";
            this.CheckCRF.UseVisualStyleBackColor = true;
            this.CheckCRF.Click += new System.EventHandler(this.CheckCRF_CheckedChanged);
            // 
            // CheckPixelRatio
            // 
            this.CheckPixelRatio.AutoSize = true;
            this.CheckPixelRatio.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.CheckPixelRatio.Location = new System.Drawing.Point(368, 130);
            this.CheckPixelRatio.Name = "CheckPixelRatio";
            this.CheckPixelRatio.Size = new System.Drawing.Size(121, 17);
            this.CheckPixelRatio.TabIndex = 40;
            this.CheckPixelRatio.Text = "Anamorphic PAR";
            this.CheckPixelRatio.UseVisualStyleBackColor = true;
            this.CheckPixelRatio.CheckedChanged += new System.EventHandler(this.CheckPixelRatio_CheckedChanged);
            // 
            // Label23
            // 
            this.Label23.AutoSize = true;
            this.Label23.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label23.Location = new System.Drawing.Point(13, 134);
            this.Label23.Name = "Label23";
            this.Label23.Size = new System.Drawing.Size(100, 13);
            this.Label23.TabIndex = 39;
            this.Label23.Text = "Video Settings";
            // 
            // Label22
            // 
            this.Label22.AutoSize = true;
            this.Label22.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label22.Location = new System.Drawing.Point(365, 13);
            this.Label22.Name = "Label22";
            this.Label22.Size = new System.Drawing.Size(175, 13);
            this.Label22.TabIndex = 38;
            this.Label22.Text = "Advanced Output Settings";
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(13, 13);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(110, 13);
            this.Label2.TabIndex = 37;
            this.Label2.Text = "Quality Settings";
            // 
            // check_grayscale
            // 
            this.check_grayscale.AutoSize = true;
            this.check_grayscale.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_grayscale.Location = new System.Drawing.Point(368, 107);
            this.check_grayscale.Name = "check_grayscale";
            this.check_grayscale.Size = new System.Drawing.Size(138, 17);
            this.check_grayscale.TabIndex = 20;
            this.check_grayscale.Text = "Grayscale Encoding";
            this.check_grayscale.UseVisualStyleBackColor = true;
            // 
            // SliderValue
            // 
            this.SliderValue.AutoSize = true;
            this.SliderValue.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.SliderValue.Location = new System.Drawing.Point(303, 96);
            this.SliderValue.Name = "SliderValue";
            this.SliderValue.Size = new System.Drawing.Size(23, 12);
            this.SliderValue.TabIndex = 32;
            this.SliderValue.Text = "0%";
            // 
            // check_DeInterlace
            // 
            this.check_DeInterlace.AutoSize = true;
            this.check_DeInterlace.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_DeInterlace.Location = new System.Drawing.Point(368, 84);
            this.check_DeInterlace.Name = "check_DeInterlace";
            this.check_DeInterlace.Size = new System.Drawing.Size(98, 17);
            this.check_DeInterlace.TabIndex = 19;
            this.check_DeInterlace.Text = "De-Interlace";
            this.check_DeInterlace.UseVisualStyleBackColor = true;
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
            this.drp_videoFramerate.Location = new System.Drawing.Point(135, 153);
            this.drp_videoFramerate.Name = "drp_videoFramerate";
            this.drp_videoFramerate.Size = new System.Drawing.Size(112, 21);
            this.drp_videoFramerate.TabIndex = 21;
            this.drp_videoFramerate.Text = "Automatic";
            // 
            // check_2PassEncode
            // 
            this.check_2PassEncode.AutoSize = true;
            this.check_2PassEncode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_2PassEncode.Location = new System.Drawing.Point(368, 38);
            this.check_2PassEncode.Name = "check_2PassEncode";
            this.check_2PassEncode.Size = new System.Drawing.Size(119, 17);
            this.check_2PassEncode.TabIndex = 18;
            this.check_2PassEncode.Text = "2-Pass Encoding";
            this.check_2PassEncode.UseVisualStyleBackColor = true;
            // 
            // slider_videoQuality
            // 
            this.slider_videoQuality.Location = new System.Drawing.Point(129, 90);
            this.slider_videoQuality.Maximum = 100;
            this.slider_videoQuality.Name = "slider_videoQuality";
            this.slider_videoQuality.Size = new System.Drawing.Size(167, 42);
            this.slider_videoQuality.TabIndex = 6;
            this.slider_videoQuality.TickFrequency = 17;
            this.slider_videoQuality.Scroll += new System.EventHandler(this.slider_videoQuality_Scroll);
            // 
            // text_filesize
            // 
            this.text_filesize.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_filesize.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_filesize.Location = new System.Drawing.Point(140, 63);
            this.text_filesize.Name = "text_filesize";
            this.text_filesize.Size = new System.Drawing.Size(156, 21);
            this.text_filesize.TabIndex = 16;
            this.text_filesize.TextChanged += new System.EventHandler(this.text_filesize_TextChanged);
            // 
            // Label46
            // 
            this.Label46.AutoSize = true;
            this.Label46.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label46.Location = new System.Drawing.Point(13, 158);
            this.Label46.Name = "Label46";
            this.Label46.Size = new System.Drawing.Size(107, 13);
            this.Label46.TabIndex = 21;
            this.Label46.Text = "Video Framerate:";
            // 
            // Label40
            // 
            this.Label40.AutoSize = true;
            this.Label40.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label40.Location = new System.Drawing.Point(13, 96);
            this.Label40.Name = "Label40";
            this.Label40.Size = new System.Drawing.Size(107, 13);
            this.Label40.TabIndex = 27;
            this.Label40.Text = "Constant Quality:";
            // 
            // text_bitrate
            // 
            this.text_bitrate.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_bitrate.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_bitrate.Location = new System.Drawing.Point(140, 36);
            this.text_bitrate.Name = "text_bitrate";
            this.text_bitrate.Size = new System.Drawing.Size(156, 21);
            this.text_bitrate.TabIndex = 15;
            this.text_bitrate.TextChanged += new System.EventHandler(this.text_bitrate_TextChanged);
            // 
            // Label42
            // 
            this.Label42.AutoSize = true;
            this.Label42.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label42.Location = new System.Drawing.Point(13, 38);
            this.Label42.Name = "Label42";
            this.Label42.Size = new System.Drawing.Size(117, 13);
            this.Label42.TabIndex = 18;
            this.Label42.Text = "Avg Bitrate (kbps):";
            // 
            // TabPage2
            // 
            this.TabPage2.BackColor = System.Drawing.SystemColors.Control;
            this.TabPage2.Controls.Add(this.Label29);
            this.TabPage2.Controls.Add(this.drp_audioMixDown);
            this.TabPage2.Controls.Add(this.drp_audioChannels);
            this.TabPage2.Controls.Add(this.drp_audioBitrate);
            this.TabPage2.Controls.Add(this.Label14);
            this.TabPage2.Controls.Add(this.Label5);
            this.TabPage2.Controls.Add(this.Label35);
            this.TabPage2.Controls.Add(this.Label16);
            this.TabPage2.Controls.Add(this.Label32);
            this.TabPage2.Controls.Add(this.Label18);
            this.TabPage2.Controls.Add(this.drp_audioSampleRate);
            this.TabPage2.Location = new System.Drawing.Point(4, 22);
            this.TabPage2.Name = "TabPage2";
            this.TabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage2.Size = new System.Drawing.Size(639, 268);
            this.TabPage2.TabIndex = 3;
            this.TabPage2.Text = "Audio Settings";
            // 
            // Label29
            // 
            this.Label29.AutoSize = true;
            this.Label29.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label29.Location = new System.Drawing.Point(155, 151);
            this.Label29.Name = "Label29";
            this.Label29.Size = new System.Drawing.Size(189, 24);
            this.Label29.TabIndex = 42;
            this.Label29.Text = "Please note: Some options require a \r\n5.1 Audio Channel to be selected.";
            // 
            // drp_audioMixDown
            // 
            this.drp_audioMixDown.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audioMixDown.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audioMixDown.FormattingEnabled = true;
            this.drp_audioMixDown.Items.AddRange(new object[] {
            "Mono",
            "Stereo",
            "Dolby Surround",
            "Dolby Pro Logic II",
            "6 Channel Discrete"});
            this.drp_audioMixDown.Location = new System.Drawing.Point(157, 127);
            this.drp_audioMixDown.Name = "drp_audioMixDown";
            this.drp_audioMixDown.Size = new System.Drawing.Size(173, 21);
            this.drp_audioMixDown.TabIndex = 50;
            this.drp_audioMixDown.Text = "Automatic";
            this.drp_audioMixDown.SelectedIndexChanged += new System.EventHandler(this.drp_audioMixDown_SelectedIndexChanged);
            // 
            // drp_audioChannels
            // 
            this.drp_audioChannels.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_audioChannels.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_audioChannels.FormattingEnabled = true;
            this.drp_audioChannels.Location = new System.Drawing.Point(157, 97);
            this.drp_audioChannels.Name = "drp_audioChannels";
            this.drp_audioChannels.Size = new System.Drawing.Size(173, 21);
            this.drp_audioChannels.TabIndex = 49;
            this.drp_audioChannels.Text = "Automatic";
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
            this.drp_audioBitrate.Location = new System.Drawing.Point(157, 37);
            this.drp_audioBitrate.Name = "drp_audioBitrate";
            this.drp_audioBitrate.Size = new System.Drawing.Size(101, 21);
            this.drp_audioBitrate.TabIndex = 40;
            this.drp_audioBitrate.Text = "128";
            // 
            // Label14
            // 
            this.Label14.AutoSize = true;
            this.Label14.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label14.Location = new System.Drawing.Point(13, 130);
            this.Label14.Name = "Label14";
            this.Label14.Size = new System.Drawing.Size(99, 13);
            this.Label14.TabIndex = 39;
            this.Label14.Text = "Audio MixDown:";
            // 
            // Label5
            // 
            this.Label5.AutoSize = true;
            this.Label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label5.Location = new System.Drawing.Point(13, 13);
            this.Label5.Name = "Label5";
            this.Label5.Size = new System.Drawing.Size(101, 13);
            this.Label5.TabIndex = 37;
            this.Label5.Text = "Audio Settings";
            // 
            // Label35
            // 
            this.Label35.AutoSize = true;
            this.Label35.Location = new System.Drawing.Point(239, 103);
            this.Label35.Name = "Label35";
            this.Label35.Size = new System.Drawing.Size(0, 13);
            this.Label35.TabIndex = 30;
            // 
            // Label16
            // 
            this.Label16.AutoSize = true;
            this.Label16.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label16.Location = new System.Drawing.Point(13, 40);
            this.Label16.Name = "Label16";
            this.Label16.Size = new System.Drawing.Size(91, 13);
            this.Label16.TabIndex = 20;
            this.Label16.Text = "Bitrate (kbps):";
            // 
            // Label32
            // 
            this.Label32.AutoSize = true;
            this.Label32.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label32.Location = new System.Drawing.Point(13, 100);
            this.Label32.Name = "Label32";
            this.Label32.Size = new System.Drawing.Size(80, 13);
            this.Label32.TabIndex = 29;
            this.Label32.Text = "Audio Track:";
            // 
            // Label18
            // 
            this.Label18.AutoSize = true;
            this.Label18.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label18.Location = new System.Drawing.Point(13, 70);
            this.Label18.Name = "Label18";
            this.Label18.Size = new System.Drawing.Size(120, 13);
            this.Label18.TabIndex = 23;
            this.Label18.Text = "Sample Rate (kHz):";
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
            this.drp_audioSampleRate.Location = new System.Drawing.Point(157, 67);
            this.drp_audioSampleRate.Name = "drp_audioSampleRate";
            this.drp_audioSampleRate.Size = new System.Drawing.Size(101, 21);
            this.drp_audioSampleRate.TabIndex = 23;
            this.drp_audioSampleRate.Text = "44.1";
            // 
            // h264Tab
            // 
            this.h264Tab.BackColor = System.Drawing.SystemColors.Control;
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
            this.h264Tab.Size = new System.Drawing.Size(639, 268);
            this.h264Tab.TabIndex = 5;
            this.h264Tab.Text = "H.264";
            // 
            // Label43
            // 
            this.Label43.AutoSize = true;
            this.Label43.Location = new System.Drawing.Point(78, 224);
            this.Label43.Name = "Label43";
            this.Label43.Size = new System.Drawing.Size(158, 13);
            this.Label43.TabIndex = 48;
            this.Label43.Text = "for help using this feature.";
            // 
            // label_h264
            // 
            this.label_h264.AutoSize = true;
            this.label_h264.Location = new System.Drawing.Point(13, 224);
            this.label_h264.Name = "label_h264";
            this.label_h264.Size = new System.Drawing.Size(66, 13);
            this.label_h264.TabIndex = 47;
            this.label_h264.TabStop = true;
            this.label_h264.Text = "Click Here";
            this.label_h264.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.label_h264_LinkClicked);
            // 
            // Label95
            // 
            this.Label95.AutoSize = true;
            this.Label95.Location = new System.Drawing.Point(13, 205);
            this.Label95.Name = "Label95";
            this.Label95.Size = new System.Drawing.Size(387, 13);
            this.Label95.TabIndex = 46;
            this.Label95.Text = "Note: Incorrect usage of this feature will cause the encoder to fail!";
            // 
            // btn_h264Clear
            // 
            this.btn_h264Clear.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_h264Clear.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_h264Clear.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_h264Clear.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_h264Clear.Location = new System.Drawing.Point(542, 50);
            this.btn_h264Clear.Name = "btn_h264Clear";
            this.btn_h264Clear.Size = new System.Drawing.Size(79, 23);
            this.btn_h264Clear.TabIndex = 45;
            this.btn_h264Clear.Text = "Clear";
            this.btn_h264Clear.UseVisualStyleBackColor = true;
            this.btn_h264Clear.Click += new System.EventHandler(this.btn_h264Clear_Click);
            // 
            // Label90
            // 
            this.Label90.AutoSize = true;
            this.Label90.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label90.Location = new System.Drawing.Point(13, 13);
            this.Label90.Name = "Label90";
            this.Label90.Size = new System.Drawing.Size(165, 13);
            this.Label90.TabIndex = 42;
            this.Label90.Text = "Advanced H.264 Options";
            // 
            // rtf_h264advanced
            // 
            this.rtf_h264advanced.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.rtf_h264advanced.Location = new System.Drawing.Point(16, 79);
            this.rtf_h264advanced.Name = "rtf_h264advanced";
            this.rtf_h264advanced.Size = new System.Drawing.Size(605, 123);
            this.rtf_h264advanced.TabIndex = 41;
            this.rtf_h264advanced.Text = "";
            // 
            // Label92
            // 
            this.Label92.AutoSize = true;
            this.Label92.Location = new System.Drawing.Point(13, 41);
            this.Label92.Name = "Label92";
            this.Label92.Size = new System.Drawing.Size(370, 26);
            this.Label92.TabIndex = 40;
            this.Label92.Text = "Specify advanced x264 options in the same style as mencoder:\r\noption1=value1:opti" +
                "on2=value2";
            // 
            // TabPage6
            // 
            this.TabPage6.BackColor = System.Drawing.SystemColors.Control;
            this.TabPage6.Controls.Add(this.Label7);
            this.TabPage6.Controls.Add(this.Label39);
            this.TabPage6.Controls.Add(this.btn_ClearQuery);
            this.TabPage6.Controls.Add(this.GenerateQuery);
            this.TabPage6.Controls.Add(this.QueryEditorText);
            this.TabPage6.Location = new System.Drawing.Point(4, 22);
            this.TabPage6.Name = "TabPage6";
            this.TabPage6.Padding = new System.Windows.Forms.Padding(3);
            this.TabPage6.Size = new System.Drawing.Size(639, 268);
            this.TabPage6.TabIndex = 6;
            this.TabPage6.Text = "Query Editor";
            // 
            // Label7
            // 
            this.Label7.AutoSize = true;
            this.Label7.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label7.Location = new System.Drawing.Point(13, 13);
            this.Label7.Name = "Label7";
            this.Label7.Size = new System.Drawing.Size(89, 13);
            this.Label7.TabIndex = 42;
            this.Label7.Text = "Query Editor";
            // 
            // Label39
            // 
            this.Label39.AutoSize = true;
            this.Label39.Location = new System.Drawing.Point(13, 34);
            this.Label39.Name = "Label39";
            this.Label39.Size = new System.Drawing.Size(403, 39);
            this.Label39.TabIndex = 40;
            this.Label39.Text = "Here you can alter the query generated by the program.\r\nClick the \"Generate Query" +
                " Now\" button to continue.\r\nRemember to re-generate the query each time you chang" +
                "e a setting!";
            // 
            // btn_ClearQuery
            // 
            this.btn_ClearQuery.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_ClearQuery.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_ClearQuery.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_ClearQuery.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_ClearQuery.Location = new System.Drawing.Point(542, 84);
            this.btn_ClearQuery.Name = "btn_ClearQuery";
            this.btn_ClearQuery.Size = new System.Drawing.Size(79, 23);
            this.btn_ClearQuery.TabIndex = 39;
            this.btn_ClearQuery.Text = "Clear";
            this.btn_ClearQuery.UseVisualStyleBackColor = true;
            this.btn_ClearQuery.Click += new System.EventHandler(this.btn_ClearQuery_Click);
            // 
            // GenerateQuery
            // 
            this.GenerateQuery.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.GenerateQuery.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.GenerateQuery.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GenerateQuery.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.GenerateQuery.Location = new System.Drawing.Point(16, 84);
            this.GenerateQuery.Name = "GenerateQuery";
            this.GenerateQuery.Size = new System.Drawing.Size(176, 23);
            this.GenerateQuery.TabIndex = 38;
            this.GenerateQuery.Text = "Generate Query Now";
            this.GenerateQuery.UseVisualStyleBackColor = true;
            this.GenerateQuery.Click += new System.EventHandler(this.GenerateQuery_Click);
            // 
            // QueryEditorText
            // 
            this.QueryEditorText.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.QueryEditorText.Location = new System.Drawing.Point(16, 114);
            this.QueryEditorText.Name = "QueryEditorText";
            this.QueryEditorText.Size = new System.Drawing.Size(605, 127);
            this.QueryEditorText.TabIndex = 41;
            this.QueryEditorText.Text = "";
            // 
            // lbl_update
            // 
            this.lbl_update.AutoSize = true;
            this.lbl_update.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_update.ForeColor = System.Drawing.Color.Black;
            this.lbl_update.Location = new System.Drawing.Point(86, 594);
            this.lbl_update.Name = "lbl_update";
            this.lbl_update.Size = new System.Drawing.Size(193, 13);
            this.lbl_update.TabIndex = 417;
            this.lbl_update.Text = "- A New Version is available!";
            this.lbl_update.Visible = false;
            // 
            // btn_queue
            // 
            this.btn_queue.BackColor = System.Drawing.SystemColors.Control;
            this.btn_queue.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_queue.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_queue.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_queue.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_queue.Location = new System.Drawing.Point(404, 589);
            this.btn_queue.Name = "btn_queue";
            this.btn_queue.Size = new System.Drawing.Size(124, 22);
            this.btn_queue.TabIndex = 416;
            this.btn_queue.TabStop = false;
            this.btn_queue.Text = "Add to Queue";
            this.btn_queue.UseVisualStyleBackColor = false;
            this.btn_queue.Click += new System.EventHandler(this.btn_queue_Click);
            // 
            // btn_encode
            // 
            this.btn_encode.BackColor = System.Drawing.SystemColors.Control;
            this.btn_encode.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_encode.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_encode.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_encode.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_encode.Location = new System.Drawing.Point(535, 589);
            this.btn_encode.Name = "btn_encode";
            this.btn_encode.Size = new System.Drawing.Size(124, 22);
            this.btn_encode.TabIndex = 414;
            this.btn_encode.TabStop = false;
            this.btn_encode.Text = "Encode Video";
            this.btn_encode.UseVisualStyleBackColor = false;
            this.btn_encode.Click += new System.EventHandler(this.btn_encode_Click);
            // 
            // Version
            // 
            this.Version.BackColor = System.Drawing.Color.Transparent;
            this.Version.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Version.Location = new System.Drawing.Point(13, 594);
            this.Version.Name = "Version";
            this.Version.Size = new System.Drawing.Size(141, 20);
            this.Version.TabIndex = 415;
            this.Version.Text = "Version 2.3";
            // 
            // lbl_chptWarn
            // 
            this.lbl_chptWarn.AutoSize = true;
            this.lbl_chptWarn.Location = new System.Drawing.Point(287, 85);
            this.lbl_chptWarn.Name = "lbl_chptWarn";
            this.lbl_chptWarn.Size = new System.Drawing.Size(77, 13);
            this.lbl_chptWarn.TabIndex = 43;
            this.lbl_chptWarn.Text = "{Warning}";
            this.lbl_chptWarn.Visible = false;
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(675, 621);
            this.Controls.Add(this.lbl_update);
            this.Controls.Add(this.btn_queue);
            this.Controls.Add(this.btn_encode);
            this.Controls.Add(this.Version);
            this.Controls.Add(this.advancedOptions);
            this.Controls.Add(this.GroupBox4);
            this.Controls.Add(this.GroupBox1);
            this.Controls.Add(this.frmMainMenu);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximumSize = new System.Drawing.Size(878, 750);
            this.MinimumSize = new System.Drawing.Size(683, 648);
            this.Name = "frmMain";
            this.Text = "Handbrake";
            this.frmMainMenu.ResumeLayout(false);
            this.frmMainMenu.PerformLayout();
            this.GroupBox1.ResumeLayout(false);
            this.GroupBox1.PerformLayout();
            this.GroupBox4.ResumeLayout(false);
            this.GroupBox4.PerformLayout();
            this.advancedOptions.ResumeLayout(false);
            this.TabPage1.ResumeLayout(false);
            this.TabPage1.PerformLayout();
            this.TabPage3.ResumeLayout(false);
            this.TabPage3.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).EndInit();
            this.TabPage2.ResumeLayout(false);
            this.TabPage2.PerformLayout();
            this.h264Tab.ResumeLayout(false);
            this.h264Tab.PerformLayout();
            this.TabPage6.ResumeLayout(false);
            this.TabPage6.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.SaveFileDialog DVD_Save;
        internal System.Windows.Forms.SaveFileDialog File_Save;
        internal System.Windows.Forms.ToolTip ToolTip;
        internal System.Windows.Forms.FolderBrowserDialog DVD_Open;
        internal System.Windows.Forms.OpenFileDialog File_Open;
        internal System.Windows.Forms.OpenFileDialog ISO_Open;
        internal System.Windows.Forms.ToolStripMenuItem FileToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_open;
        internal System.Windows.Forms.ToolStripMenuItem mnu_save;
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator1;
        internal System.Windows.Forms.ToolStripMenuItem mnu_update;
        internal System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        internal System.Windows.Forms.ToolStripMenuItem mnu_exit;
        internal System.Windows.Forms.ToolStripMenuItem ToolsToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_encode;
        internal System.Windows.Forms.ToolStripMenuItem mnu_viewDVDdata;
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator5;
        internal System.Windows.Forms.ToolStripMenuItem mnu_options;
        internal System.Windows.Forms.ToolStripMenuItem PresetsToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_preset_ipod133;
        internal System.Windows.Forms.ToolStripMenuItem mnu_preset_ipod178;
        internal System.Windows.Forms.ToolStripMenuItem mnu_preset_ipod235;
        internal System.Windows.Forms.ToolStripMenuItem mnu_appleTv;
        internal System.Windows.Forms.ToolStripMenuItem mnu_presetPS3;
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator4;
        internal System.Windows.Forms.ToolStripMenuItem mnu_ProgramDefaultOptions;
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
        internal System.Windows.Forms.GroupBox GroupBox4;
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
        internal System.Windows.Forms.TabControl advancedOptions;
        internal System.Windows.Forms.TabPage TabPage1;
        internal System.Windows.Forms.ComboBox drp_subtitle;
        internal System.Windows.Forms.Label Label19;
        internal System.Windows.Forms.Label lbl_RecomendedCrop;
        internal System.Windows.Forms.Label Label8;
        internal System.Windows.Forms.Label Label1;
        internal System.Windows.Forms.Label Label53;
        internal System.Windows.Forms.Label Label21;
        internal System.Windows.Forms.Label Label20;
        internal System.Windows.Forms.Label Label52;
        internal System.Windows.Forms.Label Label51;
        internal System.Windows.Forms.Label Label50;
        internal System.Windows.Forms.Label Label15;
        internal System.Windows.Forms.TextBox text_top;
        internal System.Windows.Forms.TextBox text_bottom;
        internal System.Windows.Forms.ComboBox drp_crop;
        internal System.Windows.Forms.TextBox text_right;
        internal System.Windows.Forms.TextBox text_left;
        internal System.Windows.Forms.TabPage TabPage3;
        internal System.Windows.Forms.Label Label41;
        internal System.Windows.Forms.Label Label37;
        internal System.Windows.Forms.CheckBox check_largeFile;
        internal System.Windows.Forms.CheckBox check_turbo;
        internal System.Windows.Forms.Label Label36;
        internal System.Windows.Forms.CheckBox Check_ChapterMarkers;
        internal System.Windows.Forms.Label Label28;
        internal System.Windows.Forms.Label Label27;
        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.CheckBox CheckCRF;
        internal System.Windows.Forms.CheckBox CheckPixelRatio;
        internal System.Windows.Forms.Label Label23;
        internal System.Windows.Forms.Label Label22;
        internal System.Windows.Forms.Label Label2;
        internal System.Windows.Forms.CheckBox check_grayscale;
        internal System.Windows.Forms.Label SliderValue;
        internal System.Windows.Forms.CheckBox check_DeInterlace;
        internal System.Windows.Forms.ComboBox drp_videoFramerate;
        internal System.Windows.Forms.CheckBox check_2PassEncode;
        internal System.Windows.Forms.TrackBar slider_videoQuality;
        internal System.Windows.Forms.TextBox text_filesize;
        internal System.Windows.Forms.Label Label46;
        internal System.Windows.Forms.Label Label40;
        internal System.Windows.Forms.TextBox text_bitrate;
        internal System.Windows.Forms.Label Label42;
        internal System.Windows.Forms.TabPage TabPage2;
        internal System.Windows.Forms.Label Label29;
        internal System.Windows.Forms.ComboBox drp_audioMixDown;
        internal System.Windows.Forms.ComboBox drp_audioChannels;
        internal System.Windows.Forms.ComboBox drp_audioBitrate;
        internal System.Windows.Forms.Label Label14;
        internal System.Windows.Forms.Label Label5;
        internal System.Windows.Forms.Label Label35;
        internal System.Windows.Forms.Label Label16;
        internal System.Windows.Forms.Label Label32;
        internal System.Windows.Forms.Label Label18;
        internal System.Windows.Forms.ComboBox drp_audioSampleRate;
        internal System.Windows.Forms.TabPage h264Tab;
        internal System.Windows.Forms.Label Label43;
        internal System.Windows.Forms.LinkLabel label_h264;
        internal System.Windows.Forms.Label Label95;
        internal System.Windows.Forms.Button btn_h264Clear;
        internal System.Windows.Forms.Label Label90;
        internal System.Windows.Forms.RichTextBox rtf_h264advanced;
        internal System.Windows.Forms.Label Label92;
        internal System.Windows.Forms.TabPage TabPage6;
        internal System.Windows.Forms.Label Label7;
        internal System.Windows.Forms.Label Label39;
        internal System.Windows.Forms.Button btn_ClearQuery;
        internal System.Windows.Forms.Button GenerateQuery;
        internal System.Windows.Forms.RichTextBox QueryEditorText;
        internal System.Windows.Forms.Label lbl_update;
        internal System.Windows.Forms.Button btn_queue;
        internal System.Windows.Forms.Button btn_encode;
        internal System.Windows.Forms.Label Version;
        private System.Windows.Forms.Label lbl_chptWarn;

    }
}