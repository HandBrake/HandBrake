/*  frmOptions.Designer.cs 
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmOptions
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmOptions));
            this.btn_close = new System.Windows.Forms.Button();
            this.drp_completeOption = new System.Windows.Forms.ComboBox();
            this.tab_options = new System.Windows.Forms.TabControl();
            this.tab_general = new System.Windows.Forms.TabPage();
            this.check_TitleCase = new System.Windows.Forms.CheckBox();
            this.check_removeUnderscores = new System.Windows.Forms.CheckBox();
            this.check_growlEncode = new System.Windows.Forms.CheckBox();
            this.check_GrowlQueue = new System.Windows.Forms.CheckBox();
            this.check_m4v = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.drop_updateCheckDays = new System.Windows.Forms.ComboBox();
            this.check_autoNaming = new System.Windows.Forms.CheckBox();
            this.txt_autoNameFormat = new System.Windows.Forms.TextBox();
            this.check_tooltip = new System.Windows.Forms.CheckBox();
            this.label5 = new System.Windows.Forms.Label();
            this.check_updateCheck = new System.Windows.Forms.CheckBox();
            this.label10 = new System.Windows.Forms.Label();
            this.btn_browse = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.text_an_path = new System.Windows.Forms.TextBox();
            this.tab_picture = new System.Windows.Forms.TabPage();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.label29 = new System.Windows.Forms.Label();
            this.btn_vlcPath = new System.Windows.Forms.Button();
            this.txt_vlcPath = new System.Windows.Forms.TextBox();
            this.tab_audio_sub = new System.Windows.Forms.TabPage();
            this.label31 = new System.Windows.Forms.Label();
            this.label15 = new System.Windows.Forms.Label();
            this.radio_foreignAndSubs = new System.Windows.Forms.RadioButton();
            this.drop_preferredLang = new System.Windows.Forms.ComboBox();
            this.radio_dub = new System.Windows.Forms.RadioButton();
            this.tab_cli = new System.Windows.Forms.TabPage();
            this.label11 = new System.Windows.Forms.Label();
            this.check_preventSleep = new System.Windows.Forms.CheckBox();
            this.check_clearOldLogs = new System.Windows.Forms.CheckBox();
            this.label12 = new System.Windows.Forms.Label();
            this.btn_viewLogs = new System.Windows.Forms.Button();
            this.label9 = new System.Windows.Forms.Label();
            this.Label4 = new System.Windows.Forms.Label();
            this.btn_clearLogs = new System.Windows.Forms.Button();
            this.drp_Priority = new System.Windows.Forms.ComboBox();
            this.check_logsInSpecifiedLocation = new System.Windows.Forms.CheckBox();
            this.check_saveLogWithVideo = new System.Windows.Forms.CheckBox();
            this.label3 = new System.Windows.Forms.Label();
            this.btn_saveLog = new System.Windows.Forms.Button();
            this.cb_logVerboseLvl = new System.Windows.Forms.ComboBox();
            this.text_logPath = new System.Windows.Forms.TextBox();
            this.label14 = new System.Windows.Forms.Label();
            this.tab_advanced = new System.Windows.Forms.TabPage();
            this.check_showCliForInGUIEncode = new System.Windows.Forms.CheckBox();
            this.drop_previewScanCount = new System.Windows.Forms.ComboBox();
            this.label33 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.check_trayStatusAlerts = new System.Windows.Forms.CheckBox();
            this.check_mainMinimize = new System.Windows.Forms.CheckBox();
            this.check_promptOnUnmatchingQueries = new System.Windows.Forms.CheckBox();
            this.check_dvdnav = new System.Windows.Forms.CheckBox();
            this.check_queryEditorTab = new System.Windows.Forms.CheckBox();
            this.label32 = new System.Windows.Forms.Label();
            this.drop_x264step = new System.Windows.Forms.ComboBox();
            this.label30 = new System.Windows.Forms.Label();
            this.check_disablePresetNotification = new System.Windows.Forms.CheckBox();
            this.label28 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.pictureBox2 = new System.Windows.Forms.PictureBox();
            this.pathFinder = new System.Windows.Forms.FolderBrowserDialog();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.textBox2 = new System.Windows.Forms.TextBox();
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.checkBox2 = new System.Windows.Forms.CheckBox();
            this.checkBox3 = new System.Windows.Forms.CheckBox();
            this.checkBox4 = new System.Windows.Forms.CheckBox();
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.textBox3 = new System.Windows.Forms.TextBox();
            this.textBox4 = new System.Windows.Forms.TextBox();
            this.checkBox5 = new System.Windows.Forms.CheckBox();
            this.checkBox6 = new System.Windows.Forms.CheckBox();
            this.checkBox7 = new System.Windows.Forms.CheckBox();
            this.checkBox8 = new System.Windows.Forms.CheckBox();
            this.comboBox2 = new System.Windows.Forms.ComboBox();
            this.label16 = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.label18 = new System.Windows.Forms.Label();
            this.label19 = new System.Windows.Forms.Label();
            this.label20 = new System.Windows.Forms.Label();
            this.label21 = new System.Windows.Forms.Label();
            this.label22 = new System.Windows.Forms.Label();
            this.label23 = new System.Windows.Forms.Label();
            this.button2 = new System.Windows.Forms.Button();
            this.label24 = new System.Windows.Forms.Label();
            this.label25 = new System.Windows.Forms.Label();
            this.label26 = new System.Windows.Forms.Label();
            this.label27 = new System.Windows.Forms.Label();
            this.openFile_vlc = new System.Windows.Forms.OpenFileDialog();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.check_AddCCTracks = new System.Windows.Forms.CheckBox();
            this.radio_preferredAudioAndSubs = new System.Windows.Forms.RadioButton();
            this.tab_options.SuspendLayout();
            this.tab_general.SuspendLayout();
            this.tab_picture.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.tab_audio_sub.SuspendLayout();
            this.tab_cli.SuspendLayout();
            this.tab_advanced.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // btn_close
            // 
            this.btn_close.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(528, 440);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(72, 22);
            this.btn_close.TabIndex = 53;
            this.btn_close.Text = "Close";
            this.btn_close.UseVisualStyleBackColor = true;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // drp_completeOption
            // 
            this.drp_completeOption.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.drp_completeOption.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_completeOption.FormattingEnabled = true;
            this.drp_completeOption.Items.AddRange(new object[] {
            "Do nothing",
            "Shutdown",
            "Suspend",
            "Hibernate",
            "Lock system",
            "Log off",
            "Quit HandBrake"});
            this.drp_completeOption.Location = new System.Drawing.Point(108, 95);
            this.drp_completeOption.Name = "drp_completeOption";
            this.drp_completeOption.Size = new System.Drawing.Size(166, 21);
            this.drp_completeOption.TabIndex = 43;
            this.ToolTip.SetToolTip(this.drp_completeOption, "Performs an action when an encode or queue has completed.");
            this.drp_completeOption.SelectedIndexChanged += new System.EventHandler(this.drp_completeOption_SelectedIndexChanged);
            // 
            // tab_options
            // 
            this.tab_options.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tab_options.Controls.Add(this.tab_general);
            this.tab_options.Controls.Add(this.tab_picture);
            this.tab_options.Controls.Add(this.tab_audio_sub);
            this.tab_options.Controls.Add(this.tab_cli);
            this.tab_options.Controls.Add(this.tab_advanced);
            this.tab_options.Location = new System.Drawing.Point(12, 74);
            this.tab_options.Name = "tab_options";
            this.tab_options.SelectedIndex = 0;
            this.tab_options.Size = new System.Drawing.Size(588, 360);
            this.tab_options.TabIndex = 58;
            // 
            // tab_general
            // 
            this.tab_general.Controls.Add(this.check_TitleCase);
            this.tab_general.Controls.Add(this.check_removeUnderscores);
            this.tab_general.Controls.Add(this.check_growlEncode);
            this.tab_general.Controls.Add(this.check_GrowlQueue);
            this.tab_general.Controls.Add(this.check_m4v);
            this.tab_general.Controls.Add(this.label1);
            this.tab_general.Controls.Add(this.label7);
            this.tab_general.Controls.Add(this.drp_completeOption);
            this.tab_general.Controls.Add(this.drop_updateCheckDays);
            this.tab_general.Controls.Add(this.check_autoNaming);
            this.tab_general.Controls.Add(this.txt_autoNameFormat);
            this.tab_general.Controls.Add(this.check_tooltip);
            this.tab_general.Controls.Add(this.label5);
            this.tab_general.Controls.Add(this.check_updateCheck);
            this.tab_general.Controls.Add(this.label10);
            this.tab_general.Controls.Add(this.btn_browse);
            this.tab_general.Controls.Add(this.label2);
            this.tab_general.Controls.Add(this.label13);
            this.tab_general.Controls.Add(this.text_an_path);
            this.tab_general.Location = new System.Drawing.Point(4, 22);
            this.tab_general.Name = "tab_general";
            this.tab_general.Padding = new System.Windows.Forms.Padding(10);
            this.tab_general.Size = new System.Drawing.Size(580, 334);
            this.tab_general.TabIndex = 3;
            this.tab_general.Text = "General";
            this.tab_general.UseVisualStyleBackColor = true;
            // 
            // check_TitleCase
            // 
            this.check_TitleCase.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_TitleCase.AutoSize = true;
            this.check_TitleCase.Location = new System.Drawing.Point(297, 279);
            this.check_TitleCase.Name = "check_TitleCase";
            this.check_TitleCase.Size = new System.Drawing.Size(151, 17);
            this.check_TitleCase.TabIndex = 101;
            this.check_TitleCase.Text = "Change case to Title Case";
            this.ToolTip.SetToolTip(this.check_TitleCase, "For the {source} option:.\r\nChange the source name to Title Case\r\ne.g \"MOVIE NAME\"" +
                    " to \"Movie Name\"\r\n");
            this.check_TitleCase.UseVisualStyleBackColor = true;
            this.check_TitleCase.CheckedChanged += new System.EventHandler(this.check_TitleCase_CheckedChanged);
            // 
            // check_removeUnderscores
            // 
            this.check_removeUnderscores.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_removeUnderscores.AutoSize = true;
            this.check_removeUnderscores.Location = new System.Drawing.Point(108, 279);
            this.check_removeUnderscores.Name = "check_removeUnderscores";
            this.check_removeUnderscores.Size = new System.Drawing.Size(183, 17);
            this.check_removeUnderscores.TabIndex = 100;
            this.check_removeUnderscores.Text = "Remove Underscores from Name";
            this.ToolTip.SetToolTip(this.check_removeUnderscores, "For the {source} option:\r\nRemove any underscores from the source name.\r\n\r\n");
            this.check_removeUnderscores.UseVisualStyleBackColor = true;
            this.check_removeUnderscores.CheckedChanged += new System.EventHandler(this.check_removeUnderscores_CheckedChanged);
            // 
            // check_growlEncode
            // 
            this.check_growlEncode.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_growlEncode.AutoSize = true;
            this.check_growlEncode.BackColor = System.Drawing.Color.Transparent;
            this.check_growlEncode.Location = new System.Drawing.Point(282, 122);
            this.check_growlEncode.Name = "check_growlEncode";
            this.check_growlEncode.Size = new System.Drawing.Size(171, 17);
            this.check_growlEncode.TabIndex = 99;
            this.check_growlEncode.Text = "Growl after Encode Completes";
            this.ToolTip.SetToolTip(this.check_growlEncode, "If you have \"Growl for Windows\" installed, you can use this feature to see growl " +
                    "alerts on your desktop.");
            this.check_growlEncode.UseVisualStyleBackColor = false;
            this.check_growlEncode.CheckedChanged += new System.EventHandler(this.check_growlEncode_CheckedChanged);
            // 
            // check_GrowlQueue
            // 
            this.check_GrowlQueue.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_GrowlQueue.AutoSize = true;
            this.check_GrowlQueue.BackColor = System.Drawing.Color.Transparent;
            this.check_GrowlQueue.Location = new System.Drawing.Point(108, 122);
            this.check_GrowlQueue.Name = "check_GrowlQueue";
            this.check_GrowlQueue.Size = new System.Drawing.Size(168, 17);
            this.check_GrowlQueue.TabIndex = 98;
            this.check_GrowlQueue.Text = "Growl after Queue Completes";
            this.ToolTip.SetToolTip(this.check_GrowlQueue, "If you have \"Growl for Windows\" installed, you can use this feature to see growl " +
                    "alerts on your desktop.");
            this.check_GrowlQueue.UseVisualStyleBackColor = false;
            this.check_GrowlQueue.CheckedChanged += new System.EventHandler(this.check_GrowlQueue_CheckedChanged);
            // 
            // check_m4v
            // 
            this.check_m4v.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_m4v.AutoSize = true;
            this.check_m4v.Location = new System.Drawing.Point(108, 303);
            this.check_m4v.Name = "check_m4v";
            this.check_m4v.Size = new System.Drawing.Size(319, 17);
            this.check_m4v.TabIndex = 82;
            this.check_m4v.Text = "Always use iPod/iTunes friendly file extension (.m4v) for MP4";
            this.ToolTip.SetToolTip(this.check_m4v, resources.GetString("check_m4v.ToolTip"));
            this.check_m4v.UseVisualStyleBackColor = true;
            this.check_m4v.CheckedChanged += new System.EventHandler(this.check_m4v_CheckedChanged);
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(15, 15);
            this.label1.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(66, 13);
            this.label1.TabIndex = 67;
            this.label1.Text = "At Launch:";
            // 
            // label7
            // 
            this.label7.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(179, 260);
            this.label7.Margin = new System.Windows.Forms.Padding(3);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(225, 13);
            this.label7.TabIndex = 81;
            this.label7.Text = "Available Options: {source} {title} {chapters}";
            // 
            // drop_updateCheckDays
            // 
            this.drop_updateCheckDays.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.drop_updateCheckDays.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_updateCheckDays.FormattingEnabled = true;
            this.drop_updateCheckDays.Items.AddRange(new object[] {
            "Daily",
            "Weekly",
            "Monthly"});
            this.drop_updateCheckDays.Location = new System.Drawing.Point(127, 35);
            this.drop_updateCheckDays.Name = "drop_updateCheckDays";
            this.drop_updateCheckDays.Size = new System.Drawing.Size(97, 21);
            this.drop_updateCheckDays.TabIndex = 97;
            this.ToolTip.SetToolTip(this.drop_updateCheckDays, "Check for updates: Daily, Weekly or Monthly\r\nDefault: Weekly");
            this.drop_updateCheckDays.SelectedIndexChanged += new System.EventHandler(this.drop_updateCheckDays_SelectedIndexChanged);
            // 
            // check_autoNaming
            // 
            this.check_autoNaming.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_autoNaming.AutoSize = true;
            this.check_autoNaming.Location = new System.Drawing.Point(108, 174);
            this.check_autoNaming.Name = "check_autoNaming";
            this.check_autoNaming.Size = new System.Drawing.Size(176, 17);
            this.check_autoNaming.TabIndex = 72;
            this.check_autoNaming.Text = "Automatically name output files";
            this.ToolTip.SetToolTip(this.check_autoNaming, "Automatically name output files");
            this.check_autoNaming.UseVisualStyleBackColor = true;
            this.check_autoNaming.CheckedChanged += new System.EventHandler(this.check_autoNaming_CheckedChanged);
            // 
            // txt_autoNameFormat
            // 
            this.txt_autoNameFormat.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.txt_autoNameFormat.Location = new System.Drawing.Point(182, 233);
            this.txt_autoNameFormat.Name = "txt_autoNameFormat";
            this.txt_autoNameFormat.Size = new System.Drawing.Size(349, 21);
            this.txt_autoNameFormat.TabIndex = 79;
            this.ToolTip.SetToolTip(this.txt_autoNameFormat, "Define the format of the automatically named file.\r\ne.g  {source}_{title}_some-te" +
                    "xt\r\n{source} {title} {chapters} will be automatically substituted for the input " +
                    "sources values.");
            this.txt_autoNameFormat.TextChanged += new System.EventHandler(this.txt_autoNameFormat_TextChanged);
            // 
            // check_tooltip
            // 
            this.check_tooltip.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_tooltip.AutoSize = true;
            this.check_tooltip.BackColor = System.Drawing.Color.Transparent;
            this.check_tooltip.Location = new System.Drawing.Point(108, 62);
            this.check_tooltip.Name = "check_tooltip";
            this.check_tooltip.Size = new System.Drawing.Size(117, 17);
            this.check_tooltip.TabIndex = 70;
            this.check_tooltip.Text = "Enable GUI tooltips";
            this.ToolTip.SetToolTip(this.check_tooltip, "Enable the built in tooltips for gui controls. (Requires Restart)");
            this.check_tooltip.UseVisualStyleBackColor = false;
            this.check_tooltip.CheckedChanged += new System.EventHandler(this.check_tooltip_CheckedChanged);
            // 
            // label5
            // 
            this.label5.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(105, 236);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(45, 13);
            this.label5.TabIndex = 80;
            this.label5.Text = "Format:";
            // 
            // check_updateCheck
            // 
            this.check_updateCheck.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_updateCheck.AutoSize = true;
            this.check_updateCheck.BackColor = System.Drawing.Color.Transparent;
            this.check_updateCheck.Location = new System.Drawing.Point(108, 14);
            this.check_updateCheck.Name = "check_updateCheck";
            this.check_updateCheck.Size = new System.Drawing.Size(114, 17);
            this.check_updateCheck.TabIndex = 68;
            this.check_updateCheck.Text = "Check for updates";
            this.ToolTip.SetToolTip(this.check_updateCheck, "Enables the built in update checker. This check is performed when the application" +
                    " starts.");
            this.check_updateCheck.UseVisualStyleBackColor = false;
            this.check_updateCheck.CheckedChanged += new System.EventHandler(this.check_updateCheck_CheckedChanged);
            // 
            // label10
            // 
            this.label10.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(105, 209);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(71, 13);
            this.label10.TabIndex = 77;
            this.label10.Text = "Default Path:";
            // 
            // btn_browse
            // 
            this.btn_browse.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.btn_browse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_browse.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_browse.Location = new System.Drawing.Point(456, 204);
            this.btn_browse.Name = "btn_browse";
            this.btn_browse.Size = new System.Drawing.Size(75, 23);
            this.btn_browse.TabIndex = 78;
            this.btn_browse.Text = "Browse";
            this.btn_browse.UseVisualStyleBackColor = true;
            this.btn_browse.Click += new System.EventHandler(this.btn_browse_Click);
            // 
            // label2
            // 
            this.label2.Anchor = System.Windows.Forms.AnchorStyles.Right;
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(7, 98);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(74, 13);
            this.label2.TabIndex = 54;
            this.label2.Text = "When Done:";
            // 
            // label13
            // 
            this.label13.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label13.AutoSize = true;
            this.label13.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label13.Location = new System.Drawing.Point(4, 164);
            this.label13.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(77, 13);
            this.label13.TabIndex = 71;
            this.label13.Text = "Output Files:";
            // 
            // text_an_path
            // 
            this.text_an_path.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.text_an_path.Location = new System.Drawing.Point(182, 206);
            this.text_an_path.Name = "text_an_path";
            this.text_an_path.Size = new System.Drawing.Size(253, 21);
            this.text_an_path.TabIndex = 76;
            this.ToolTip.SetToolTip(this.text_an_path, "This is the default location where your encoded files will be stored if \"Automati" +
                    "cally name output files\" is enabled.");
            this.text_an_path.TextChanged += new System.EventHandler(this.text_an_path_TextChanged);
            // 
            // tab_picture
            // 
            this.tab_picture.Controls.Add(this.tableLayoutPanel2);
            this.tab_picture.Location = new System.Drawing.Point(4, 22);
            this.tab_picture.Name = "tab_picture";
            this.tab_picture.Padding = new System.Windows.Forms.Padding(10);
            this.tab_picture.Size = new System.Drawing.Size(580, 334);
            this.tab_picture.TabIndex = 5;
            this.tab_picture.Text = "Picture";
            this.tab_picture.UseVisualStyleBackColor = true;
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.AutoSize = true;
            this.tableLayoutPanel2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tableLayoutPanel2.ColumnCount = 4;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel2.Controls.Add(this.label29, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.btn_vlcPath, 3, 0);
            this.tableLayoutPanel2.Controls.Add(this.txt_vlcPath, 2, 0);
            this.tableLayoutPanel2.Location = new System.Drawing.Point(10, 10);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 2;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(560, 28);
            this.tableLayoutPanel2.TabIndex = 84;
            // 
            // label29
            // 
            this.label29.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label29.AutoSize = true;
            this.label29.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label29.Location = new System.Drawing.Point(3, 5);
            this.label29.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label29.Name = "label29";
            this.tableLayoutPanel2.SetRowSpan(this.label29, 2);
            this.label29.Size = new System.Drawing.Size(59, 13);
            this.label29.TabIndex = 79;
            this.label29.Text = "VLC Path:";
            // 
            // btn_vlcPath
            // 
            this.btn_vlcPath.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.btn_vlcPath.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_vlcPath.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_vlcPath.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_vlcPath.Location = new System.Drawing.Point(489, 3);
            this.btn_vlcPath.Name = "btn_vlcPath";
            this.btn_vlcPath.Size = new System.Drawing.Size(68, 22);
            this.btn_vlcPath.TabIndex = 83;
            this.btn_vlcPath.Text = "Browse";
            this.btn_vlcPath.UseVisualStyleBackColor = true;
            this.btn_vlcPath.Click += new System.EventHandler(this.btn_vlcPath_Click);
            // 
            // txt_vlcPath
            // 
            this.txt_vlcPath.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.txt_vlcPath.Location = new System.Drawing.Point(88, 3);
            this.txt_vlcPath.Name = "txt_vlcPath";
            this.txt_vlcPath.Size = new System.Drawing.Size(395, 21);
            this.txt_vlcPath.TabIndex = 81;
            this.ToolTip.SetToolTip(this.txt_vlcPath, "The path where VLC is installed on this system.\r\nThis is used for the video previ" +
                    "ew feature.");
            this.txt_vlcPath.TextChanged += new System.EventHandler(this.txt_vlcPath_TextChanged);
            // 
            // tab_audio_sub
            // 
            this.tab_audio_sub.Controls.Add(this.radio_preferredAudioAndSubs);
            this.tab_audio_sub.Controls.Add(this.check_AddCCTracks);
            this.tab_audio_sub.Controls.Add(this.label31);
            this.tab_audio_sub.Controls.Add(this.label15);
            this.tab_audio_sub.Controls.Add(this.radio_foreignAndSubs);
            this.tab_audio_sub.Controls.Add(this.drop_preferredLang);
            this.tab_audio_sub.Controls.Add(this.radio_dub);
            this.tab_audio_sub.Location = new System.Drawing.Point(4, 22);
            this.tab_audio_sub.Name = "tab_audio_sub";
            this.tab_audio_sub.Padding = new System.Windows.Forms.Padding(3);
            this.tab_audio_sub.Size = new System.Drawing.Size(580, 334);
            this.tab_audio_sub.TabIndex = 6;
            this.tab_audio_sub.Text = "Audio / Subtitles";
            this.tab_audio_sub.UseVisualStyleBackColor = true;
            // 
            // label31
            // 
            this.label31.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label31.AutoSize = true;
            this.label31.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label31.Location = new System.Drawing.Point(16, 19);
            this.label31.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label31.Name = "label31";
            this.label31.Size = new System.Drawing.Size(52, 13);
            this.label31.TabIndex = 91;
            this.label31.Text = "Actions:";
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(87, 19);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(107, 13);
            this.label15.TabIndex = 88;
            this.label15.Text = "Preferred Language:";
            // 
            // radio_foreignAndSubs
            // 
            this.radio_foreignAndSubs.AutoSize = true;
            this.radio_foreignAndSubs.Location = new System.Drawing.Point(200, 76);
            this.radio_foreignAndSubs.Name = "radio_foreignAndSubs";
            this.radio_foreignAndSubs.Size = new System.Drawing.Size(358, 17);
            this.radio_foreignAndSubs.TabIndex = 90;
            this.radio_foreignAndSubs.TabStop = true;
            this.radio_foreignAndSubs.Text = "Use foreign language audio and add first preferred language subtitle.";
            this.ToolTip.SetToolTip(this.radio_foreignAndSubs, "Use foreign language audio and add first subtitle track of prefered language.\r\n");
            this.radio_foreignAndSubs.UseVisualStyleBackColor = true;
            this.radio_foreignAndSubs.CheckedChanged += new System.EventHandler(this.radio_foreignAndSubs_CheckedChanged);
            // 
            // drop_preferredLang
            // 
            this.drop_preferredLang.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_preferredLang.FormattingEnabled = true;
            this.drop_preferredLang.Location = new System.Drawing.Point(200, 16);
            this.drop_preferredLang.Name = "drop_preferredLang";
            this.drop_preferredLang.Size = new System.Drawing.Size(118, 21);
            this.drop_preferredLang.TabIndex = 87;
            this.ToolTip.SetToolTip(this.drop_preferredLang, "The number of processor\'s / processor cores. Unless your having problems, leave o" +
                    "n Automatic.");
            this.drop_preferredLang.SelectedIndexChanged += new System.EventHandler(this.drop_preferredLang_SelectedIndexChanged);
            // 
            // radio_dub
            // 
            this.radio_dub.AutoSize = true;
            this.radio_dub.Location = new System.Drawing.Point(200, 53);
            this.radio_dub.Name = "radio_dub";
            this.radio_dub.Size = new System.Drawing.Size(164, 17);
            this.radio_dub.TabIndex = 89;
            this.radio_dub.TabStop = true;
            this.radio_dub.Text = "DUB Foreign Language Audio";
            this.ToolTip.SetToolTip(this.radio_dub, "This option will select your prefered language of audio for the source if one exi" +
                    "sts.");
            this.radio_dub.UseVisualStyleBackColor = true;
            this.radio_dub.CheckedChanged += new System.EventHandler(this.radio_dub_CheckedChanged);
            // 
            // tab_cli
            // 
            this.tab_cli.Controls.Add(this.label11);
            this.tab_cli.Controls.Add(this.check_preventSleep);
            this.tab_cli.Controls.Add(this.check_clearOldLogs);
            this.tab_cli.Controls.Add(this.label12);
            this.tab_cli.Controls.Add(this.btn_viewLogs);
            this.tab_cli.Controls.Add(this.label9);
            this.tab_cli.Controls.Add(this.Label4);
            this.tab_cli.Controls.Add(this.btn_clearLogs);
            this.tab_cli.Controls.Add(this.drp_Priority);
            this.tab_cli.Controls.Add(this.check_logsInSpecifiedLocation);
            this.tab_cli.Controls.Add(this.check_saveLogWithVideo);
            this.tab_cli.Controls.Add(this.label3);
            this.tab_cli.Controls.Add(this.btn_saveLog);
            this.tab_cli.Controls.Add(this.cb_logVerboseLvl);
            this.tab_cli.Controls.Add(this.text_logPath);
            this.tab_cli.Controls.Add(this.label14);
            this.tab_cli.Location = new System.Drawing.Point(4, 22);
            this.tab_cli.Name = "tab_cli";
            this.tab_cli.Padding = new System.Windows.Forms.Padding(10);
            this.tab_cli.Size = new System.Drawing.Size(580, 334);
            this.tab_cli.TabIndex = 2;
            this.tab_cli.Text = "CLI / System / Logging";
            this.tab_cli.UseVisualStyleBackColor = true;
            // 
            // label11
            // 
            this.label11.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label11.AutoSize = true;
            this.label11.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label11.Location = new System.Drawing.Point(7, 46);
            this.label11.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(53, 13);
            this.label11.TabIndex = 92;
            this.label11.Text = "System:";
            // 
            // check_preventSleep
            // 
            this.check_preventSleep.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_preventSleep.AutoSize = true;
            this.check_preventSleep.Location = new System.Drawing.Point(73, 45);
            this.check_preventSleep.Name = "check_preventSleep";
            this.check_preventSleep.Size = new System.Drawing.Size(272, 17);
            this.check_preventSleep.TabIndex = 91;
            this.check_preventSleep.Text = "Prevent the system from sleepping when encoding.";
            this.ToolTip.SetToolTip(this.check_preventSleep, "Prevent system from sleeping during encoding.");
            this.check_preventSleep.UseVisualStyleBackColor = true;
            this.check_preventSleep.CheckedChanged += new System.EventHandler(this.check_preventSleep_CheckedChanged);
            // 
            // check_clearOldLogs
            // 
            this.check_clearOldLogs.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_clearOldLogs.AutoSize = true;
            this.check_clearOldLogs.Location = new System.Drawing.Point(73, 234);
            this.check_clearOldLogs.Name = "check_clearOldLogs";
            this.check_clearOldLogs.Size = new System.Drawing.Size(166, 17);
            this.check_clearOldLogs.TabIndex = 90;
            this.check_clearOldLogs.Text = "Clear logs older than 30 days";
            this.ToolTip.SetToolTip(this.check_clearOldLogs, "Clear logs which are older than 30 days.\r\nThis only applies to HandBrakes Applica" +
                    "tion Data Log folder.");
            this.check_clearOldLogs.UseVisualStyleBackColor = true;
            this.check_clearOldLogs.CheckedChanged += new System.EventHandler(this.check_clearOldLogs_CheckedChanged);
            // 
            // label12
            // 
            this.label12.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label12.AutoSize = true;
            this.label12.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label12.Location = new System.Drawing.Point(7, 15);
            this.label12.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(28, 13);
            this.label12.TabIndex = 75;
            this.label12.Text = "CLI:";
            // 
            // btn_viewLogs
            // 
            this.btn_viewLogs.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.btn_viewLogs.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_viewLogs.ForeColor = System.Drawing.Color.DarkOrange;
            this.btn_viewLogs.Location = new System.Drawing.Point(129, 196);
            this.btn_viewLogs.Name = "btn_viewLogs";
            this.btn_viewLogs.Size = new System.Drawing.Size(139, 23);
            this.btn_viewLogs.TabIndex = 89;
            this.btn_viewLogs.Text = "View Log Directory";
            this.btn_viewLogs.UseVisualStyleBackColor = true;
            this.btn_viewLogs.Click += new System.EventHandler(this.btn_viewLogs_Click);
            // 
            // label9
            // 
            this.label9.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label9.Location = new System.Drawing.Point(7, 92);
            this.label9.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(36, 13);
            this.label9.TabIndex = 77;
            this.label9.Text = "Logs:";
            // 
            // Label4
            // 
            this.Label4.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.Label4.AutoSize = true;
            this.Label4.BackColor = System.Drawing.Color.Transparent;
            this.Label4.Location = new System.Drawing.Point(70, 15);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(70, 13);
            this.Label4.TabIndex = 42;
            this.Label4.Text = "Priority level:";
            // 
            // btn_clearLogs
            // 
            this.btn_clearLogs.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.btn_clearLogs.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_clearLogs.ForeColor = System.Drawing.Color.DarkOrange;
            this.btn_clearLogs.Location = new System.Drawing.Point(274, 196);
            this.btn_clearLogs.Name = "btn_clearLogs";
            this.btn_clearLogs.Size = new System.Drawing.Size(135, 23);
            this.btn_clearLogs.TabIndex = 88;
            this.btn_clearLogs.Text = "Clear Log History";
            this.btn_clearLogs.UseVisualStyleBackColor = true;
            this.btn_clearLogs.Click += new System.EventHandler(this.btn_clearLogs_Click);
            // 
            // drp_Priority
            // 
            this.drp_Priority.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.drp_Priority.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_Priority.FormattingEnabled = true;
            this.drp_Priority.Items.AddRange(new object[] {
            "Realtime",
            "High",
            "Above Normal",
            "Normal",
            "Below Normal",
            "Low"});
            this.drp_Priority.Location = new System.Drawing.Point(177, 12);
            this.drp_Priority.Name = "drp_Priority";
            this.drp_Priority.Size = new System.Drawing.Size(111, 21);
            this.drp_Priority.TabIndex = 43;
            this.ToolTip.SetToolTip(this.drp_Priority, "Set the application priority level for the CLI. \r\nIt\'s best to leave this on Belo" +
                    "w Normal if you wish to use your system whilst encoding with HandBrake.\r\n");
            this.drp_Priority.SelectedIndexChanged += new System.EventHandler(this.drp_Priority_SelectedIndexChanged);
            // 
            // check_logsInSpecifiedLocation
            // 
            this.check_logsInSpecifiedLocation.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_logsInSpecifiedLocation.AutoSize = true;
            this.check_logsInSpecifiedLocation.Location = new System.Drawing.Point(73, 139);
            this.check_logsInSpecifiedLocation.Name = "check_logsInSpecifiedLocation";
            this.check_logsInSpecifiedLocation.Size = new System.Drawing.Size(306, 17);
            this.check_logsInSpecifiedLocation.TabIndex = 87;
            this.check_logsInSpecifiedLocation.Text = "Put a copy of individual encode logs in a specified location:";
            this.ToolTip.SetToolTip(this.check_logsInSpecifiedLocation, "Place a copy of the encode log in the same folder as the encoded movie.");
            this.check_logsInSpecifiedLocation.UseVisualStyleBackColor = true;
            this.check_logsInSpecifiedLocation.CheckedChanged += new System.EventHandler(this.check_logsInSpecifiedLocation_CheckedChanged);
            // 
            // check_saveLogWithVideo
            // 
            this.check_saveLogWithVideo.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_saveLogWithVideo.AutoSize = true;
            this.check_saveLogWithVideo.Location = new System.Drawing.Point(73, 116);
            this.check_saveLogWithVideo.Name = "check_saveLogWithVideo";
            this.check_saveLogWithVideo.Size = new System.Drawing.Size(382, 17);
            this.check_saveLogWithVideo.TabIndex = 83;
            this.check_saveLogWithVideo.Text = "Put a copy of individual encode logs in the same location as encoded video";
            this.ToolTip.SetToolTip(this.check_saveLogWithVideo, "Place a copy of the encode log in the same folder as the encoded movie.");
            this.check_saveLogWithVideo.UseVisualStyleBackColor = true;
            this.check_saveLogWithVideo.CheckedChanged += new System.EventHandler(this.check_saveLogWithVideo_CheckedChanged);
            // 
            // label3
            // 
            this.label3.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label3.AutoSize = true;
            this.label3.BackColor = System.Drawing.Color.Transparent;
            this.label3.Location = new System.Drawing.Point(70, 92);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(101, 13);
            this.label3.TabIndex = 85;
            this.label3.Text = "Log verbosity level:";
            // 
            // btn_saveLog
            // 
            this.btn_saveLog.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.btn_saveLog.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_saveLog.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_saveLog.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_saveLog.Location = new System.Drawing.Point(437, 169);
            this.btn_saveLog.Name = "btn_saveLog";
            this.btn_saveLog.Size = new System.Drawing.Size(77, 22);
            this.btn_saveLog.TabIndex = 82;
            this.btn_saveLog.Text = "Browse";
            this.btn_saveLog.UseVisualStyleBackColor = true;
            this.btn_saveLog.Click += new System.EventHandler(this.btn_saveLog_Click);
            // 
            // cb_logVerboseLvl
            // 
            this.cb_logVerboseLvl.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.cb_logVerboseLvl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cb_logVerboseLvl.FormattingEnabled = true;
            this.cb_logVerboseLvl.Items.AddRange(new object[] {
            "0",
            "1",
            "2"});
            this.cb_logVerboseLvl.Location = new System.Drawing.Point(177, 89);
            this.cb_logVerboseLvl.Name = "cb_logVerboseLvl";
            this.cb_logVerboseLvl.Size = new System.Drawing.Size(111, 21);
            this.cb_logVerboseLvl.TabIndex = 86;
            this.ToolTip.SetToolTip(this.cb_logVerboseLvl, "Activity Log Verbosity Level");
            this.cb_logVerboseLvl.SelectedIndexChanged += new System.EventHandler(this.cb_logVerboseLvl_SelectedIndexChanged);
            // 
            // text_logPath
            // 
            this.text_logPath.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.text_logPath.Location = new System.Drawing.Point(129, 169);
            this.text_logPath.Name = "text_logPath";
            this.text_logPath.Size = new System.Drawing.Size(302, 21);
            this.text_logPath.TabIndex = 80;
            this.ToolTip.SetToolTip(this.text_logPath, "The default location where auto named files are stored.");
            this.text_logPath.TextChanged += new System.EventHandler(this.text_logPath_TextChanged);
            // 
            // label14
            // 
            this.label14.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(70, 172);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(53, 13);
            this.label14.TabIndex = 81;
            this.label14.Text = "Log Path:";
            // 
            // tab_advanced
            // 
            this.tab_advanced.Controls.Add(this.check_showCliForInGUIEncode);
            this.tab_advanced.Controls.Add(this.drop_previewScanCount);
            this.tab_advanced.Controls.Add(this.label33);
            this.tab_advanced.Controls.Add(this.label6);
            this.tab_advanced.Controls.Add(this.check_trayStatusAlerts);
            this.tab_advanced.Controls.Add(this.check_mainMinimize);
            this.tab_advanced.Controls.Add(this.check_promptOnUnmatchingQueries);
            this.tab_advanced.Controls.Add(this.check_dvdnav);
            this.tab_advanced.Controls.Add(this.check_queryEditorTab);
            this.tab_advanced.Controls.Add(this.label32);
            this.tab_advanced.Controls.Add(this.drop_x264step);
            this.tab_advanced.Controls.Add(this.label30);
            this.tab_advanced.Controls.Add(this.check_disablePresetNotification);
            this.tab_advanced.Controls.Add(this.label28);
            this.tab_advanced.Location = new System.Drawing.Point(4, 22);
            this.tab_advanced.Name = "tab_advanced";
            this.tab_advanced.Padding = new System.Windows.Forms.Padding(10);
            this.tab_advanced.Size = new System.Drawing.Size(580, 334);
            this.tab_advanced.TabIndex = 4;
            this.tab_advanced.Text = "Advanced / Other";
            this.tab_advanced.UseVisualStyleBackColor = true;
            // 
            // check_showCliForInGUIEncode
            // 
            this.check_showCliForInGUIEncode.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_showCliForInGUIEncode.AutoSize = true;
            this.check_showCliForInGUIEncode.BackColor = System.Drawing.Color.Transparent;
            this.check_showCliForInGUIEncode.Location = new System.Drawing.Point(81, 129);
            this.check_showCliForInGUIEncode.Name = "check_showCliForInGUIEncode";
            this.check_showCliForInGUIEncode.Size = new System.Drawing.Size(330, 17);
            this.check_showCliForInGUIEncode.TabIndex = 96;
            this.check_showCliForInGUIEncode.Text = "Show CLI window (Allows you to cleanly exit encode with ctrl-c)";
            this.ToolTip.SetToolTip(this.check_showCliForInGUIEncode, resources.GetString("check_showCliForInGUIEncode.ToolTip"));
            this.check_showCliForInGUIEncode.UseVisualStyleBackColor = false;
            this.check_showCliForInGUIEncode.CheckedChanged += new System.EventHandler(this.check_showCliForInGUIEncode_CheckedChanged);
            // 
            // drop_previewScanCount
            // 
            this.drop_previewScanCount.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_previewScanCount.FormattingEnabled = true;
            this.drop_previewScanCount.Items.AddRange(new object[] {
            "10",
            "15",
            "20",
            "25",
            "30"});
            this.drop_previewScanCount.Location = new System.Drawing.Point(273, 173);
            this.drop_previewScanCount.Name = "drop_previewScanCount";
            this.drop_previewScanCount.Size = new System.Drawing.Size(85, 21);
            this.drop_previewScanCount.TabIndex = 95;
            this.ToolTip.SetToolTip(this.drop_previewScanCount, "Allows a greater number of preview frames to be selectable on the \"Preview\" windo" +
                    "w.\r\n\r\nThis can in some cases improve the accuracy of the auto crop feature.");
            this.drop_previewScanCount.SelectedIndexChanged += new System.EventHandler(this.drop_previewScanCount_SelectedIndexChanged);
            // 
            // label33
            // 
            this.label33.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label33.AutoSize = true;
            this.label33.Location = new System.Drawing.Point(79, 176);
            this.label33.Name = "label33";
            this.label33.Size = new System.Drawing.Size(181, 13);
            this.label33.TabIndex = 94;
            this.label33.Text = "Number of picture previews to scan:";
            // 
            // label6
            // 
            this.label6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(15, 15);
            this.label6.Margin = new System.Windows.Forms.Padding(3, 5, 3, 0);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(31, 13);
            this.label6.TabIndex = 71;
            this.label6.Text = "GUI:";
            // 
            // check_trayStatusAlerts
            // 
            this.check_trayStatusAlerts.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_trayStatusAlerts.AutoSize = true;
            this.check_trayStatusAlerts.BackColor = System.Drawing.Color.Transparent;
            this.check_trayStatusAlerts.Location = new System.Drawing.Point(81, 37);
            this.check_trayStatusAlerts.Name = "check_trayStatusAlerts";
            this.check_trayStatusAlerts.Size = new System.Drawing.Size(296, 17);
            this.check_trayStatusAlerts.TabIndex = 93;
            this.check_trayStatusAlerts.Text = "Display status messages from tray icon (balloon popups)";
            this.ToolTip.SetToolTip(this.check_trayStatusAlerts, "Minimize the window to the system tray rather than the task bar.\r\nThe system tray" +
                    " icon has encode status notifications.\r\nNote: requires restart to take effect!\r\n" +
                    "");
            this.check_trayStatusAlerts.UseVisualStyleBackColor = false;
            this.check_trayStatusAlerts.CheckedChanged += new System.EventHandler(this.check_trayStatusAlerts_CheckedChanged);
            // 
            // check_mainMinimize
            // 
            this.check_mainMinimize.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_mainMinimize.AutoSize = true;
            this.check_mainMinimize.BackColor = System.Drawing.Color.Transparent;
            this.check_mainMinimize.Location = new System.Drawing.Point(81, 14);
            this.check_mainMinimize.Name = "check_mainMinimize";
            this.check_mainMinimize.Size = new System.Drawing.Size(230, 17);
            this.check_mainMinimize.TabIndex = 82;
            this.check_mainMinimize.Text = "Minimize to system tray (Requires Restart)";
            this.ToolTip.SetToolTip(this.check_mainMinimize, "Minimize the window to the system tray rather than the task bar.\r\nThe system tray" +
                    " icon has encode status notifications.\r\nNote: requires restart to take effect!\r\n" +
                    "");
            this.check_mainMinimize.UseVisualStyleBackColor = false;
            this.check_mainMinimize.CheckedChanged += new System.EventHandler(this.check_mainMinimize_CheckedChanged);
            // 
            // check_promptOnUnmatchingQueries
            // 
            this.check_promptOnUnmatchingQueries.AutoSize = true;
            this.check_promptOnUnmatchingQueries.Location = new System.Drawing.Point(100, 83);
            this.check_promptOnUnmatchingQueries.Name = "check_promptOnUnmatchingQueries";
            this.check_promptOnUnmatchingQueries.Size = new System.Drawing.Size(305, 17);
            this.check_promptOnUnmatchingQueries.TabIndex = 63;
            this.check_promptOnUnmatchingQueries.Text = "Prompt when a manual query does not match GUI settings";
            this.check_promptOnUnmatchingQueries.UseVisualStyleBackColor = true;
            this.check_promptOnUnmatchingQueries.CheckedChanged += new System.EventHandler(this.check_promptOnUnmatchingQueries_CheckedChanged);
            // 
            // check_dvdnav
            // 
            this.check_dvdnav.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_dvdnav.AutoSize = true;
            this.check_dvdnav.BackColor = System.Drawing.Color.Transparent;
            this.check_dvdnav.Location = new System.Drawing.Point(82, 247);
            this.check_dvdnav.Name = "check_dvdnav";
            this.check_dvdnav.Size = new System.Drawing.Size(276, 17);
            this.check_dvdnav.TabIndex = 90;
            this.check_dvdnav.Text = "Disable LibDVDNav. (libdvdread will be used instead)";
            this.check_dvdnav.UseVisualStyleBackColor = false;
            this.check_dvdnav.CheckedChanged += new System.EventHandler(this.check_dvdnav_CheckedChanged);
            // 
            // check_queryEditorTab
            // 
            this.check_queryEditorTab.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_queryEditorTab.AutoSize = true;
            this.check_queryEditorTab.BackColor = System.Drawing.Color.Transparent;
            this.check_queryEditorTab.Location = new System.Drawing.Point(81, 60);
            this.check_queryEditorTab.Name = "check_queryEditorTab";
            this.check_queryEditorTab.Size = new System.Drawing.Size(241, 17);
            this.check_queryEditorTab.TabIndex = 84;
            this.check_queryEditorTab.Text = "Enable \"Query Editor\" tab (Requires Restart)";
            this.ToolTip.SetToolTip(this.check_queryEditorTab, "Enables the Query Editor tab on the main window. Requires program restart to take" +
                    " effect.");
            this.check_queryEditorTab.UseVisualStyleBackColor = false;
            this.check_queryEditorTab.CheckedChanged += new System.EventHandler(this.check_queryEditorTab_CheckedChanged);
            // 
            // label32
            // 
            this.label32.Anchor = System.Windows.Forms.AnchorStyles.Right;
            this.label32.AutoSize = true;
            this.label32.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label32.Location = new System.Drawing.Point(13, 248);
            this.label32.Name = "label32";
            this.label32.Size = new System.Drawing.Size(33, 13);
            this.label32.TabIndex = 89;
            this.label32.Text = "DVD:";
            // 
            // drop_x264step
            // 
            this.drop_x264step.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_x264step.FormattingEnabled = true;
            this.drop_x264step.Items.AddRange(new object[] {
            "1.0",
            "0.50",
            "0.25",
            "0.20"});
            this.drop_x264step.Location = new System.Drawing.Point(273, 210);
            this.drop_x264step.Name = "drop_x264step";
            this.drop_x264step.Size = new System.Drawing.Size(85, 21);
            this.drop_x264step.TabIndex = 86;
            this.ToolTip.SetToolTip(this.drop_x264step, "Quality Slider Control Granularity for x264 encoding only.");
            this.drop_x264step.SelectedIndexChanged += new System.EventHandler(this.x264step_SelectedIndexChanged);
            // 
            // label30
            // 
            this.label30.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label30.AutoSize = true;
            this.label30.Location = new System.Drawing.Point(78, 213);
            this.label30.Name = "label30";
            this.label30.Size = new System.Drawing.Size(189, 13);
            this.label30.TabIndex = 87;
            this.label30.Text = "Constant quality fractional granularity";
            // 
            // check_disablePresetNotification
            // 
            this.check_disablePresetNotification.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_disablePresetNotification.AutoSize = true;
            this.check_disablePresetNotification.BackColor = System.Drawing.Color.Transparent;
            this.check_disablePresetNotification.Location = new System.Drawing.Point(81, 106);
            this.check_disablePresetNotification.Name = "check_disablePresetNotification";
            this.check_disablePresetNotification.Size = new System.Drawing.Size(222, 17);
            this.check_disablePresetNotification.TabIndex = 91;
            this.check_disablePresetNotification.Text = "Disable built-in preset update notification";
            this.ToolTip.SetToolTip(this.check_disablePresetNotification, "Disables the notification you recieve when presets are updated when a new version" +
                    " of HandBrake is installed.");
            this.check_disablePresetNotification.UseVisualStyleBackColor = false;
            this.check_disablePresetNotification.CheckedChanged += new System.EventHandler(this.check_disablePresetNotification_CheckedChanged);
            // 
            // label28
            // 
            this.label28.Anchor = System.Windows.Forms.AnchorStyles.Right;
            this.label28.AutoSize = true;
            this.label28.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label28.Location = new System.Drawing.Point(8, 213);
            this.label28.Name = "label28";
            this.label28.Size = new System.Drawing.Size(38, 13);
            this.label28.TabIndex = 85;
            this.label28.Text = "x264:";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(51, 19);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(115, 13);
            this.label8.TabIndex = 61;
            this.label8.Text = "HandBrake Options";
            // 
            // pictureBox2
            // 
            this.pictureBox2.Image = global::Handbrake.Properties.Resources.General_Preferences;
            this.pictureBox2.Location = new System.Drawing.Point(12, 9);
            this.pictureBox2.Margin = new System.Windows.Forms.Padding(3, 0, 3, 3);
            this.pictureBox2.Name = "pictureBox2";
            this.pictureBox2.Size = new System.Drawing.Size(32, 32);
            this.pictureBox2.TabIndex = 60;
            this.pictureBox2.TabStop = false;
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 1000;
            this.ToolTip.AutoPopDelay = 15000;
            this.ToolTip.InitialDelay = 1000;
            this.ToolTip.ReshowDelay = 200;
            this.ToolTip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.ToolTip.ToolTipTitle = "Tooltip";
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(199, 198);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(255, 20);
            this.textBox1.TabIndex = 79;
            this.ToolTip.SetToolTip(this.textBox1, "Define the format of the automatically named file.\r\ne.g  {source}_{title}_some-te" +
                    "xt\r\n{source} {title} {chapters} will be automatically substituted for the input " +
                    "sources values.");
            // 
            // textBox2
            // 
            this.textBox2.Location = new System.Drawing.Point(199, 171);
            this.textBox2.Name = "textBox2";
            this.textBox2.Size = new System.Drawing.Size(181, 20);
            this.textBox2.TabIndex = 76;
            this.ToolTip.SetToolTip(this.textBox2, "The default location where auto named files are stored.");
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox1.Location = new System.Drawing.Point(114, 148);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(206, 17);
            this.checkBox1.TabIndex = 72;
            this.checkBox1.Text = "Automatically name output files";
            this.ToolTip.SetToolTip(this.checkBox1, "Automatically name output files");
            this.checkBox1.UseVisualStyleBackColor = true;
            // 
            // checkBox2
            // 
            this.checkBox2.AutoSize = true;
            this.checkBox2.BackColor = System.Drawing.Color.Transparent;
            this.checkBox2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox2.Location = new System.Drawing.Point(114, 41);
            this.checkBox2.Name = "checkBox2";
            this.checkBox2.Size = new System.Drawing.Size(135, 17);
            this.checkBox2.TabIndex = 70;
            this.checkBox2.Text = "Enable GUI tooltips";
            this.ToolTip.SetToolTip(this.checkBox2, "Enable the built in tooltips for gui controls.");
            this.checkBox2.UseVisualStyleBackColor = false;
            // 
            // checkBox3
            // 
            this.checkBox3.AutoSize = true;
            this.checkBox3.BackColor = System.Drawing.Color.Transparent;
            this.checkBox3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox3.Location = new System.Drawing.Point(114, 18);
            this.checkBox3.Name = "checkBox3";
            this.checkBox3.Size = new System.Drawing.Size(131, 17);
            this.checkBox3.TabIndex = 68;
            this.checkBox3.Text = "Check for updates";
            this.ToolTip.SetToolTip(this.checkBox3, "Enables the built in update checker. This check is performed when the application" +
                    " starts.");
            this.checkBox3.UseVisualStyleBackColor = false;
            // 
            // checkBox4
            // 
            this.checkBox4.AutoSize = true;
            this.checkBox4.BackColor = System.Drawing.Color.Transparent;
            this.checkBox4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox4.Location = new System.Drawing.Point(114, 64);
            this.checkBox4.Name = "checkBox4";
            this.checkBox4.Size = new System.Drawing.Size(166, 17);
            this.checkBox4.TabIndex = 69;
            this.checkBox4.Text = "Load my default settings";
            this.ToolTip.SetToolTip(this.checkBox4, "Loads the users default settings rather than the Normal preset.");
            this.checkBox4.UseVisualStyleBackColor = false;
            // 
            // comboBox1
            // 
            this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.comboBox1.FormattingEnabled = true;
            this.comboBox1.Items.AddRange(new object[] {
            "Do Nothing",
            "Shutdown",
            "Suspend",
            "Hibernate",
            "Lock System",
            "Log Off",
            "Quit HandBrake"});
            this.comboBox1.Location = new System.Drawing.Point(114, 100);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(166, 21);
            this.comboBox1.TabIndex = 43;
            this.ToolTip.SetToolTip(this.comboBox1, "Performs an action when an encode or queue has completed.");
            // 
            // textBox3
            // 
            this.textBox3.Location = new System.Drawing.Point(199, 198);
            this.textBox3.Name = "textBox3";
            this.textBox3.Size = new System.Drawing.Size(255, 20);
            this.textBox3.TabIndex = 79;
            this.ToolTip.SetToolTip(this.textBox3, "Define the format of the automatically named file.\r\ne.g  {source}_{title}_some-te" +
                    "xt\r\n{source} {title} {chapters} will be automatically substituted for the input " +
                    "sources values.");
            // 
            // textBox4
            // 
            this.textBox4.Location = new System.Drawing.Point(199, 171);
            this.textBox4.Name = "textBox4";
            this.textBox4.Size = new System.Drawing.Size(181, 20);
            this.textBox4.TabIndex = 76;
            this.ToolTip.SetToolTip(this.textBox4, "The default location where auto named files are stored.");
            // 
            // checkBox5
            // 
            this.checkBox5.AutoSize = true;
            this.checkBox5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox5.Location = new System.Drawing.Point(114, 148);
            this.checkBox5.Name = "checkBox5";
            this.checkBox5.Size = new System.Drawing.Size(206, 17);
            this.checkBox5.TabIndex = 72;
            this.checkBox5.Text = "Automatically name output files";
            this.ToolTip.SetToolTip(this.checkBox5, "Automatically name output files");
            this.checkBox5.UseVisualStyleBackColor = true;
            // 
            // checkBox6
            // 
            this.checkBox6.AutoSize = true;
            this.checkBox6.BackColor = System.Drawing.Color.Transparent;
            this.checkBox6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox6.Location = new System.Drawing.Point(114, 41);
            this.checkBox6.Name = "checkBox6";
            this.checkBox6.Size = new System.Drawing.Size(135, 17);
            this.checkBox6.TabIndex = 70;
            this.checkBox6.Text = "Enable GUI tooltips";
            this.ToolTip.SetToolTip(this.checkBox6, "Enable the built in tooltips for gui controls.");
            this.checkBox6.UseVisualStyleBackColor = false;
            // 
            // checkBox7
            // 
            this.checkBox7.AutoSize = true;
            this.checkBox7.BackColor = System.Drawing.Color.Transparent;
            this.checkBox7.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox7.Location = new System.Drawing.Point(114, 18);
            this.checkBox7.Name = "checkBox7";
            this.checkBox7.Size = new System.Drawing.Size(131, 17);
            this.checkBox7.TabIndex = 68;
            this.checkBox7.Text = "Check for updates";
            this.ToolTip.SetToolTip(this.checkBox7, "Enables the built in update checker. This check is performed when the application" +
                    " starts.");
            this.checkBox7.UseVisualStyleBackColor = false;
            // 
            // checkBox8
            // 
            this.checkBox8.AutoSize = true;
            this.checkBox8.BackColor = System.Drawing.Color.Transparent;
            this.checkBox8.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.checkBox8.Location = new System.Drawing.Point(114, 64);
            this.checkBox8.Name = "checkBox8";
            this.checkBox8.Size = new System.Drawing.Size(166, 17);
            this.checkBox8.TabIndex = 69;
            this.checkBox8.Text = "Load my default settings";
            this.ToolTip.SetToolTip(this.checkBox8, "Loads the users default settings rather than the Normal preset.");
            this.checkBox8.UseVisualStyleBackColor = false;
            // 
            // comboBox2
            // 
            this.comboBox2.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBox2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.comboBox2.FormattingEnabled = true;
            this.comboBox2.Items.AddRange(new object[] {
            "Do Nothing",
            "Shutdown",
            "Suspend",
            "Hibernate",
            "Lock System",
            "Log Off",
            "Quit HandBrake"});
            this.comboBox2.Location = new System.Drawing.Point(114, 100);
            this.comboBox2.Name = "comboBox2";
            this.comboBox2.Size = new System.Drawing.Size(166, 21);
            this.comboBox2.TabIndex = 43;
            this.ToolTip.SetToolTip(this.comboBox2, "Performs an action when an encode or queue has completed.");
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label16.Location = new System.Drawing.Point(197, 222);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(242, 12);
            this.label16.TabIndex = 81;
            this.label16.Text = "Available Options: {source} {title} {chapters}";
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label17.Location = new System.Drawing.Point(111, 201);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(52, 13);
            this.label17.TabIndex = 80;
            this.label17.Text = "Format:";
            // 
            // button1
            // 
            this.button1.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.button1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button1.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.button1.Location = new System.Drawing.Point(386, 171);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(68, 22);
            this.button1.TabIndex = 78;
            this.button1.Text = "Browse";
            this.button1.UseVisualStyleBackColor = true;
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label18.Location = new System.Drawing.Point(111, 174);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(82, 13);
            this.label18.TabIndex = 77;
            this.label18.Text = "Default Path:";
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label19.Location = new System.Drawing.Point(10, 149);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(86, 13);
            this.label19.TabIndex = 71;
            this.label19.Text = "Output files:";
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.BackColor = System.Drawing.Color.Transparent;
            this.label20.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label20.Location = new System.Drawing.Point(21, 19);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(75, 13);
            this.label20.TabIndex = 67;
            this.label20.Text = "At Launch:";
            // 
            // label21
            // 
            this.label21.AutoSize = true;
            this.label21.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label21.Location = new System.Drawing.Point(12, 103);
            this.label21.Name = "label21";
            this.label21.Size = new System.Drawing.Size(84, 13);
            this.label21.TabIndex = 54;
            this.label21.Text = "When Done:";
            // 
            // label22
            // 
            this.label22.AutoSize = true;
            this.label22.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label22.Location = new System.Drawing.Point(197, 222);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(242, 12);
            this.label22.TabIndex = 81;
            this.label22.Text = "Available Options: {source} {title} {chapters}";
            // 
            // label23
            // 
            this.label23.AutoSize = true;
            this.label23.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label23.Location = new System.Drawing.Point(111, 201);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(52, 13);
            this.label23.TabIndex = 80;
            this.label23.Text = "Format:";
            // 
            // button2
            // 
            this.button2.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.button2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.button2.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.button2.Location = new System.Drawing.Point(386, 171);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(68, 22);
            this.button2.TabIndex = 78;
            this.button2.Text = "Browse";
            this.button2.UseVisualStyleBackColor = true;
            // 
            // label24
            // 
            this.label24.AutoSize = true;
            this.label24.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label24.Location = new System.Drawing.Point(111, 174);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(82, 13);
            this.label24.TabIndex = 77;
            this.label24.Text = "Default Path:";
            // 
            // label25
            // 
            this.label25.AutoSize = true;
            this.label25.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label25.Location = new System.Drawing.Point(10, 149);
            this.label25.Name = "label25";
            this.label25.Size = new System.Drawing.Size(86, 13);
            this.label25.TabIndex = 71;
            this.label25.Text = "Output files:";
            // 
            // label26
            // 
            this.label26.AutoSize = true;
            this.label26.BackColor = System.Drawing.Color.Transparent;
            this.label26.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label26.Location = new System.Drawing.Point(21, 19);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(75, 13);
            this.label26.TabIndex = 67;
            this.label26.Text = "At Launch:";
            // 
            // label27
            // 
            this.label27.AutoSize = true;
            this.label27.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label27.Location = new System.Drawing.Point(12, 103);
            this.label27.Name = "label27";
            this.label27.Size = new System.Drawing.Size(84, 13);
            this.label27.TabIndex = 54;
            this.label27.Text = "When Done:";
            // 
            // openFile_vlc
            // 
            this.openFile_vlc.DefaultExt = "exe";
            this.openFile_vlc.Filter = "exe|*.exe";
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.White;
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Controls.Add(this.label8);
            this.panel1.Controls.Add(this.pictureBox2);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Margin = new System.Windows.Forms.Padding(0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(615, 65);
            this.panel1.TabIndex = 62;
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.SystemColors.Control;
            this.panel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel2.Location = new System.Drawing.Point(0, 55);
            this.panel2.MaximumSize = new System.Drawing.Size(0, 10);
            this.panel2.MinimumSize = new System.Drawing.Size(0, 10);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(615, 10);
            this.panel2.TabIndex = 59;
            // 
            // check_AddCCTracks
            // 
            this.check_AddCCTracks.AutoSize = true;
            this.check_AddCCTracks.Location = new System.Drawing.Point(200, 131);
            this.check_AddCCTracks.Name = "check_AddCCTracks";
            this.check_AddCCTracks.Size = new System.Drawing.Size(199, 17);
            this.check_AddCCTracks.TabIndex = 92;
            this.check_AddCCTracks.Text = "Add Closed Captions when available";
            this.ToolTip.SetToolTip(this.check_AddCCTracks, "Add any CC tracks if they exist");
            this.check_AddCCTracks.UseVisualStyleBackColor = true;
            this.check_AddCCTracks.CheckedChanged += new System.EventHandler(this.check_AddCCTracks_CheckedChanged);
            // 
            // radio_preferredAudioAndSubs
            // 
            this.radio_preferredAudioAndSubs.AutoSize = true;
            this.radio_preferredAudioAndSubs.Location = new System.Drawing.Point(200, 99);
            this.radio_preferredAudioAndSubs.Name = "radio_preferredAudioAndSubs";
            this.radio_preferredAudioAndSubs.Size = new System.Drawing.Size(254, 17);
            this.radio_preferredAudioAndSubs.TabIndex = 93;
            this.radio_preferredAudioAndSubs.TabStop = true;
            this.radio_preferredAudioAndSubs.Text = "Use Preferred Language for Audio and Subtitles";
            this.ToolTip.SetToolTip(this.radio_preferredAudioAndSubs, "Use Preferred Language for Audio and Subtitle.");
            this.radio_preferredAudioAndSubs.UseVisualStyleBackColor = true;
            this.radio_preferredAudioAndSubs.CheckedChanged += new System.EventHandler(this.radio_preferredAudioAndSubs_CheckedChanged);
            // 
            // frmOptions
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.ClientSize = new System.Drawing.Size(615, 472);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.btn_close);
            this.Controls.Add(this.tab_options);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmOptions";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "HandBrake Options";
            this.tab_options.ResumeLayout(false);
            this.tab_general.ResumeLayout(false);
            this.tab_general.PerformLayout();
            this.tab_picture.ResumeLayout(false);
            this.tab_picture.PerformLayout();
            this.tableLayoutPanel2.ResumeLayout(false);
            this.tableLayoutPanel2.PerformLayout();
            this.tab_audio_sub.ResumeLayout(false);
            this.tab_audio_sub.PerformLayout();
            this.tab_cli.ResumeLayout(false);
            this.tab_cli.PerformLayout();
            this.tab_advanced.ResumeLayout(false);
            this.tab_advanced.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.ComboBox drp_completeOption;
        private System.Windows.Forms.TabControl tab_options;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TabPage tab_cli;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.PictureBox pictureBox2;
        private System.Windows.Forms.TabPage tab_general;
        private System.Windows.Forms.FolderBrowserDialog pathFinder;
        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.CheckBox check_tooltip;
        internal System.Windows.Forms.CheckBox check_updateCheck;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TabPage tab_advanced;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label13;
        internal System.Windows.Forms.Label label10;
        private System.Windows.Forms.TextBox text_an_path;
        internal System.Windows.Forms.CheckBox check_autoNaming;
        private System.Windows.Forms.Label label12;
        internal System.Windows.Forms.ToolTip ToolTip;
        private System.Windows.Forms.TabPage tab_picture;
        internal System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txt_autoNameFormat;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label9;
        internal System.Windows.Forms.Button btn_saveLog;
        internal System.Windows.Forms.Label label14;
        private System.Windows.Forms.TextBox text_logPath;
        internal System.Windows.Forms.CheckBox check_saveLogWithVideo;
        internal System.Windows.Forms.Button btn_vlcPath;
        private System.Windows.Forms.TextBox txt_vlcPath;
        private System.Windows.Forms.Label label29;
        private System.Windows.Forms.Label label16;
        internal System.Windows.Forms.Label label17;
        private System.Windows.Forms.TextBox textBox1;
        internal System.Windows.Forms.Button button1;
        internal System.Windows.Forms.Label label18;
        private System.Windows.Forms.TextBox textBox2;
        internal System.Windows.Forms.CheckBox checkBox1;
        private System.Windows.Forms.Label label19;
        internal System.Windows.Forms.CheckBox checkBox2;
        internal System.Windows.Forms.CheckBox checkBox3;
        internal System.Windows.Forms.CheckBox checkBox4;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.Label label21;
        internal System.Windows.Forms.ComboBox comboBox1;
        private System.Windows.Forms.Label label22;
        internal System.Windows.Forms.Label label23;
        private System.Windows.Forms.TextBox textBox3;
        internal System.Windows.Forms.Button button2;
        internal System.Windows.Forms.Label label24;
        private System.Windows.Forms.TextBox textBox4;
        internal System.Windows.Forms.CheckBox checkBox5;
        private System.Windows.Forms.Label label25;
        internal System.Windows.Forms.CheckBox checkBox6;
        internal System.Windows.Forms.CheckBox checkBox7;
        internal System.Windows.Forms.CheckBox checkBox8;
        private System.Windows.Forms.Label label26;
        private System.Windows.Forms.Label label27;
        internal System.Windows.Forms.ComboBox comboBox2;
        private System.Windows.Forms.OpenFileDialog openFile_vlc;
        internal System.Windows.Forms.CheckBox check_mainMinimize;
        internal System.Windows.Forms.CheckBox check_queryEditorTab;
        private System.Windows.Forms.Label label30;
        internal System.Windows.Forms.ComboBox drop_x264step;
        private System.Windows.Forms.Label label28;
        internal System.Windows.Forms.ComboBox cb_logVerboseLvl;
        internal System.Windows.Forms.Label label3;
        internal System.Windows.Forms.CheckBox check_dvdnav;
        private System.Windows.Forms.Label label32;
        internal System.Windows.Forms.CheckBox check_logsInSpecifiedLocation;
        internal System.Windows.Forms.CheckBox check_disablePresetNotification;
        internal System.Windows.Forms.CheckBox check_trayStatusAlerts;
        private System.Windows.Forms.Button btn_viewLogs;
        private System.Windows.Forms.Button btn_clearLogs;
        internal System.Windows.Forms.CheckBox check_m4v;
        internal System.Windows.Forms.ComboBox drop_updateCheckDays;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        internal System.Windows.Forms.ComboBox drp_Priority;
        private System.Windows.Forms.CheckBox check_promptOnUnmatchingQueries;
        private System.Windows.Forms.TabPage tab_audio_sub;
        internal System.Windows.Forms.ComboBox drop_preferredLang;
        private System.Windows.Forms.Label label31;
        private System.Windows.Forms.RadioButton radio_foreignAndSubs;
        private System.Windows.Forms.RadioButton radio_dub;
        private System.Windows.Forms.Label label15;
        internal System.Windows.Forms.Button btn_browse;
        internal System.Windows.Forms.CheckBox check_growlEncode;
        internal System.Windows.Forms.CheckBox check_GrowlQueue;
        internal System.Windows.Forms.ComboBox drop_previewScanCount;
        private System.Windows.Forms.Label label33;
        internal System.Windows.Forms.CheckBox check_clearOldLogs;
        internal System.Windows.Forms.CheckBox check_showCliForInGUIEncode;
        internal System.Windows.Forms.CheckBox check_TitleCase;
        internal System.Windows.Forms.CheckBox check_removeUnderscores;
        internal System.Windows.Forms.CheckBox check_preventSleep;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.RadioButton radio_preferredAudioAndSubs;
        private System.Windows.Forms.CheckBox check_AddCCTracks;
    }
}