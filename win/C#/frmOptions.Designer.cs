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
            this.label7 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.txt_autoNameFormat = new System.Windows.Forms.TextBox();
            this.btn_browse = new System.Windows.Forms.Button();
            this.label10 = new System.Windows.Forms.Label();
            this.text_an_path = new System.Windows.Forms.TextBox();
            this.check_autoNaming = new System.Windows.Forms.CheckBox();
            this.label13 = new System.Windows.Forms.Label();
            this.check_tooltip = new System.Windows.Forms.CheckBox();
            this.check_updateCheck = new System.Windows.Forms.CheckBox();
            this.check_userDefaultSettings = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.tab_picture = new System.Windows.Forms.TabPage();
            this.btn_vlcPath = new System.Windows.Forms.Button();
            this.txt_vlcPath = new System.Windows.Forms.TextBox();
            this.label29 = new System.Windows.Forms.Label();
            this.txt_decomb = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.tab_cli = new System.Windows.Forms.TabPage();
            this.label15 = new System.Windows.Forms.Label();
            this.check_saveLogWithVideo = new System.Windows.Forms.CheckBox();
            this.btn_saveLog = new System.Windows.Forms.Button();
            this.label14 = new System.Windows.Forms.Label();
            this.text_logPath = new System.Windows.Forms.TextBox();
            this.check_keepLogs = new System.Windows.Forms.CheckBox();
            this.label9 = new System.Windows.Forms.Label();
            this.check_cli_minimized = new System.Windows.Forms.CheckBox();
            this.label12 = new System.Windows.Forms.Label();
            this.drp_Priority = new System.Windows.Forms.ComboBox();
            this.Label11 = new System.Windows.Forms.Label();
            this.drp_processors = new System.Windows.Forms.ComboBox();
            this.Label4 = new System.Windows.Forms.Label();
            this.tab_advanced = new System.Windows.Forms.TabPage();
            this.check_snapshot = new System.Windows.Forms.CheckBox();
            this.btn_drive_detect = new System.Windows.Forms.CheckBox();
            this.label6 = new System.Windows.Forms.Label();
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
            this.check_mainMinimize = new System.Windows.Forms.CheckBox();
            this.lbl_appcastUnstable = new System.Windows.Forms.Label();
            this.tab_options.SuspendLayout();
            this.tab_general.SuspendLayout();
            this.tab_picture.SuspendLayout();
            this.tab_cli.SuspendLayout();
            this.tab_advanced.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
            this.SuspendLayout();
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(430, 346);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(72, 22);
            this.btn_close.TabIndex = 53;
            this.btn_close.Text = "Close";
            this.btn_close.UseVisualStyleBackColor = true;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // drp_completeOption
            // 
            this.drp_completeOption.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_completeOption.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_completeOption.FormattingEnabled = true;
            this.drp_completeOption.Items.AddRange(new object[] {
            "Do Nothing",
            "Shutdown",
            "Suspend",
            "Hibernate",
            "Lock System",
            "Log Off",
            "Quit HandBrake"});
            this.drp_completeOption.Location = new System.Drawing.Point(114, 100);
            this.drp_completeOption.Name = "drp_completeOption";
            this.drp_completeOption.Size = new System.Drawing.Size(166, 21);
            this.drp_completeOption.TabIndex = 43;
            this.ToolTip.SetToolTip(this.drp_completeOption, "Performs an action when an encode or queue has completed.");
            this.drp_completeOption.SelectedIndexChanged += new System.EventHandler(this.drp_completeOption_SelectedIndexChanged);
            // 
            // tab_options
            // 
            this.tab_options.Controls.Add(this.tab_general);
            this.tab_options.Controls.Add(this.tab_picture);
            this.tab_options.Controls.Add(this.tab_cli);
            this.tab_options.Controls.Add(this.tab_advanced);
            this.tab_options.Location = new System.Drawing.Point(12, 55);
            this.tab_options.Name = "tab_options";
            this.tab_options.SelectedIndex = 0;
            this.tab_options.Size = new System.Drawing.Size(490, 285);
            this.tab_options.TabIndex = 58;
            // 
            // tab_general
            // 
            this.tab_general.Controls.Add(this.label7);
            this.tab_general.Controls.Add(this.label5);
            this.tab_general.Controls.Add(this.txt_autoNameFormat);
            this.tab_general.Controls.Add(this.btn_browse);
            this.tab_general.Controls.Add(this.label10);
            this.tab_general.Controls.Add(this.text_an_path);
            this.tab_general.Controls.Add(this.check_autoNaming);
            this.tab_general.Controls.Add(this.label13);
            this.tab_general.Controls.Add(this.check_tooltip);
            this.tab_general.Controls.Add(this.check_updateCheck);
            this.tab_general.Controls.Add(this.check_userDefaultSettings);
            this.tab_general.Controls.Add(this.label1);
            this.tab_general.Controls.Add(this.label2);
            this.tab_general.Controls.Add(this.drp_completeOption);
            this.tab_general.Location = new System.Drawing.Point(4, 22);
            this.tab_general.Name = "tab_general";
            this.tab_general.Size = new System.Drawing.Size(482, 259);
            this.tab_general.TabIndex = 3;
            this.tab_general.Text = "General";
            this.tab_general.UseVisualStyleBackColor = true;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Font = new System.Drawing.Font("Verdana", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label7.Location = new System.Drawing.Point(197, 222);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(242, 12);
            this.label7.TabIndex = 81;
            this.label7.Text = "Available Options: {source} {title} {chapters}";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(111, 201);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(52, 13);
            this.label5.TabIndex = 80;
            this.label5.Text = "Format:";
            // 
            // txt_autoNameFormat
            // 
            this.txt_autoNameFormat.Location = new System.Drawing.Point(199, 198);
            this.txt_autoNameFormat.Name = "txt_autoNameFormat";
            this.txt_autoNameFormat.Size = new System.Drawing.Size(255, 21);
            this.txt_autoNameFormat.TabIndex = 79;
            this.ToolTip.SetToolTip(this.txt_autoNameFormat, "Define the format of the automatically named file.\r\ne.g  {source}_{title}_some-te" +
                    "xt\r\n{source} {title} {chapters} will be automatically substituted for the input " +
                    "sources values.");
            this.txt_autoNameFormat.TextChanged += new System.EventHandler(this.txt_autoNameFormat_TextChanged);
            // 
            // btn_browse
            // 
            this.btn_browse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_browse.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_browse.Location = new System.Drawing.Point(386, 171);
            this.btn_browse.Name = "btn_browse";
            this.btn_browse.Size = new System.Drawing.Size(68, 22);
            this.btn_browse.TabIndex = 78;
            this.btn_browse.Text = "Browse";
            this.btn_browse.UseVisualStyleBackColor = true;
            this.btn_browse.Click += new System.EventHandler(this.btn_browse_Click);
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label10.Location = new System.Drawing.Point(111, 174);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(82, 13);
            this.label10.TabIndex = 77;
            this.label10.Text = "Default Path:";
            // 
            // text_an_path
            // 
            this.text_an_path.Location = new System.Drawing.Point(199, 171);
            this.text_an_path.Name = "text_an_path";
            this.text_an_path.Size = new System.Drawing.Size(181, 21);
            this.text_an_path.TabIndex = 76;
            this.ToolTip.SetToolTip(this.text_an_path, "The default location where auto named files are stored.");
            this.text_an_path.TextChanged += new System.EventHandler(this.text_an_path_TextChanged);
            // 
            // check_autoNaming
            // 
            this.check_autoNaming.AutoSize = true;
            this.check_autoNaming.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_autoNaming.Location = new System.Drawing.Point(114, 148);
            this.check_autoNaming.Name = "check_autoNaming";
            this.check_autoNaming.Size = new System.Drawing.Size(206, 17);
            this.check_autoNaming.TabIndex = 72;
            this.check_autoNaming.Text = "Automatically name output files";
            this.ToolTip.SetToolTip(this.check_autoNaming, "Automatically name output files");
            this.check_autoNaming.UseVisualStyleBackColor = true;
            this.check_autoNaming.CheckedChanged += new System.EventHandler(this.check_autoNaming_CheckedChanged);
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label13.Location = new System.Drawing.Point(10, 149);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(86, 13);
            this.label13.TabIndex = 71;
            this.label13.Text = "Output files:";
            // 
            // check_tooltip
            // 
            this.check_tooltip.AutoSize = true;
            this.check_tooltip.BackColor = System.Drawing.Color.Transparent;
            this.check_tooltip.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_tooltip.Location = new System.Drawing.Point(114, 41);
            this.check_tooltip.Name = "check_tooltip";
            this.check_tooltip.Size = new System.Drawing.Size(135, 17);
            this.check_tooltip.TabIndex = 70;
            this.check_tooltip.Text = "Enable GUI tooltips";
            this.ToolTip.SetToolTip(this.check_tooltip, "Enable the built in tooltips for gui controls.");
            this.check_tooltip.UseVisualStyleBackColor = false;
            this.check_tooltip.CheckedChanged += new System.EventHandler(this.check_tooltip_CheckedChanged);
            // 
            // check_updateCheck
            // 
            this.check_updateCheck.AutoSize = true;
            this.check_updateCheck.BackColor = System.Drawing.Color.Transparent;
            this.check_updateCheck.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_updateCheck.Location = new System.Drawing.Point(114, 18);
            this.check_updateCheck.Name = "check_updateCheck";
            this.check_updateCheck.Size = new System.Drawing.Size(131, 17);
            this.check_updateCheck.TabIndex = 68;
            this.check_updateCheck.Text = "Check for updates";
            this.ToolTip.SetToolTip(this.check_updateCheck, "Enables the built in update checker. This check is performed when the application" +
                    " starts.");
            this.check_updateCheck.UseVisualStyleBackColor = false;
            this.check_updateCheck.CheckedChanged += new System.EventHandler(this.check_updateCheck_CheckedChanged);
            // 
            // check_userDefaultSettings
            // 
            this.check_userDefaultSettings.AutoSize = true;
            this.check_userDefaultSettings.BackColor = System.Drawing.Color.Transparent;
            this.check_userDefaultSettings.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_userDefaultSettings.Location = new System.Drawing.Point(114, 64);
            this.check_userDefaultSettings.Name = "check_userDefaultSettings";
            this.check_userDefaultSettings.Size = new System.Drawing.Size(166, 17);
            this.check_userDefaultSettings.TabIndex = 69;
            this.check_userDefaultSettings.Text = "Load my default settings";
            this.ToolTip.SetToolTip(this.check_userDefaultSettings, "Loads the users default settings rather than the Normal preset.");
            this.check_userDefaultSettings.UseVisualStyleBackColor = false;
            this.check_userDefaultSettings.CheckedChanged += new System.EventHandler(this.check_userDefaultSettings_CheckedChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(21, 19);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(75, 13);
            this.label1.TabIndex = 67;
            this.label1.Text = "At Launch:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(12, 103);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(84, 13);
            this.label2.TabIndex = 54;
            this.label2.Text = "When Done:";
            // 
            // tab_picture
            // 
            this.tab_picture.Controls.Add(this.btn_vlcPath);
            this.tab_picture.Controls.Add(this.txt_vlcPath);
            this.tab_picture.Controls.Add(this.label29);
            this.tab_picture.Controls.Add(this.txt_decomb);
            this.tab_picture.Controls.Add(this.label3);
            this.tab_picture.Location = new System.Drawing.Point(4, 22);
            this.tab_picture.Name = "tab_picture";
            this.tab_picture.Size = new System.Drawing.Size(482, 259);
            this.tab_picture.TabIndex = 5;
            this.tab_picture.Text = "Picture";
            this.tab_picture.UseVisualStyleBackColor = true;
            // 
            // btn_vlcPath
            // 
            this.btn_vlcPath.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_vlcPath.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_vlcPath.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_vlcPath.Location = new System.Drawing.Point(398, 56);
            this.btn_vlcPath.Name = "btn_vlcPath";
            this.btn_vlcPath.Size = new System.Drawing.Size(68, 22);
            this.btn_vlcPath.TabIndex = 83;
            this.btn_vlcPath.Text = "Browse";
            this.btn_vlcPath.UseVisualStyleBackColor = true;
            this.btn_vlcPath.Click += new System.EventHandler(this.btn_vlcPath_Click);
            // 
            // txt_vlcPath
            // 
            this.txt_vlcPath.Location = new System.Drawing.Point(98, 56);
            this.txt_vlcPath.Name = "txt_vlcPath";
            this.txt_vlcPath.Size = new System.Drawing.Size(294, 21);
            this.txt_vlcPath.TabIndex = 81;
            this.ToolTip.SetToolTip(this.txt_vlcPath, "The default location where auto named files are stored.");
            this.txt_vlcPath.TextChanged += new System.EventHandler(this.txt_vlcPath_TextChanged);
            // 
            // label29
            // 
            this.label29.AutoSize = true;
            this.label29.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label29.Location = new System.Drawing.Point(17, 61);
            this.label29.Name = "label29";
            this.label29.Size = new System.Drawing.Size(67, 13);
            this.label29.TabIndex = 79;
            this.label29.Text = "VLC Path:";
            // 
            // txt_decomb
            // 
            this.txt_decomb.Location = new System.Drawing.Point(98, 16);
            this.txt_decomb.Name = "txt_decomb";
            this.txt_decomb.Size = new System.Drawing.Size(181, 21);
            this.txt_decomb.TabIndex = 78;
            this.ToolTip.SetToolTip(this.txt_decomb, "Default: 4:10:15:9:10:35:9");
            this.txt_decomb.TextChanged += new System.EventHandler(this.txt_decomb_TextChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(21, 19);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(63, 13);
            this.label3.TabIndex = 77;
            this.label3.Text = "Decomb:";
            // 
            // tab_cli
            // 
            this.tab_cli.Controls.Add(this.label15);
            this.tab_cli.Controls.Add(this.check_saveLogWithVideo);
            this.tab_cli.Controls.Add(this.btn_saveLog);
            this.tab_cli.Controls.Add(this.label14);
            this.tab_cli.Controls.Add(this.text_logPath);
            this.tab_cli.Controls.Add(this.check_keepLogs);
            this.tab_cli.Controls.Add(this.label9);
            this.tab_cli.Controls.Add(this.check_cli_minimized);
            this.tab_cli.Controls.Add(this.label12);
            this.tab_cli.Controls.Add(this.drp_Priority);
            this.tab_cli.Controls.Add(this.Label11);
            this.tab_cli.Controls.Add(this.drp_processors);
            this.tab_cli.Controls.Add(this.Label4);
            this.tab_cli.Location = new System.Drawing.Point(4, 22);
            this.tab_cli.Name = "tab_cli";
            this.tab_cli.Size = new System.Drawing.Size(482, 259);
            this.tab_cli.TabIndex = 2;
            this.tab_cli.Text = "CLI";
            this.tab_cli.UseVisualStyleBackColor = true;
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label15.Location = new System.Drawing.Point(195, 197);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(24, 13);
            this.label15.TabIndex = 84;
            this.label15.Text = "OR";
            // 
            // check_saveLogWithVideo
            // 
            this.check_saveLogWithVideo.AutoSize = true;
            this.check_saveLogWithVideo.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_saveLogWithVideo.Location = new System.Drawing.Point(71, 177);
            this.check_saveLogWithVideo.Name = "check_saveLogWithVideo";
            this.check_saveLogWithVideo.Size = new System.Drawing.Size(349, 17);
            this.check_saveLogWithVideo.TabIndex = 83;
            this.check_saveLogWithVideo.Text = "Put individual encode logs in the same location as movie";
            this.ToolTip.SetToolTip(this.check_saveLogWithVideo, "Place a copy of the encode log in the same folder as the encoded movie.");
            this.check_saveLogWithVideo.UseVisualStyleBackColor = true;
            this.check_saveLogWithVideo.CheckedChanged += new System.EventHandler(this.check_saveLogWithVideo_CheckedChanged);
            // 
            // btn_saveLog
            // 
            this.btn_saveLog.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_saveLog.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_saveLog.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_saveLog.Location = new System.Drawing.Point(343, 218);
            this.btn_saveLog.Name = "btn_saveLog";
            this.btn_saveLog.Size = new System.Drawing.Size(68, 22);
            this.btn_saveLog.TabIndex = 82;
            this.btn_saveLog.Text = "Browse";
            this.btn_saveLog.UseVisualStyleBackColor = true;
            this.btn_saveLog.Click += new System.EventHandler(this.btn_saveLog_Click);
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label14.Location = new System.Drawing.Point(68, 221);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(61, 13);
            this.label14.TabIndex = 81;
            this.label14.Text = "Log Path:";
            // 
            // text_logPath
            // 
            this.text_logPath.Location = new System.Drawing.Point(135, 218);
            this.text_logPath.Name = "text_logPath";
            this.text_logPath.Size = new System.Drawing.Size(202, 21);
            this.text_logPath.TabIndex = 80;
            this.ToolTip.SetToolTip(this.text_logPath, "The default location where auto named files are stored.");
            this.text_logPath.TextChanged += new System.EventHandler(this.text_logPath_TextChanged);
            // 
            // check_keepLogs
            // 
            this.check_keepLogs.AutoSize = true;
            this.check_keepLogs.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_keepLogs.Location = new System.Drawing.Point(71, 135);
            this.check_keepLogs.Name = "check_keepLogs";
            this.check_keepLogs.Size = new System.Drawing.Size(185, 17);
            this.check_keepLogs.TabIndex = 79;
            this.check_keepLogs.Text = "Keep individual encode logs";
            this.ToolTip.SetToolTip(this.check_keepLogs, "Save encode logs to a file after the encode has completed.");
            this.check_keepLogs.UseVisualStyleBackColor = true;
            this.check_keepLogs.CheckedChanged += new System.EventHandler(this.check_keepLogs_CheckedChanged);
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label9.Location = new System.Drawing.Point(12, 136);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(41, 13);
            this.label9.TabIndex = 77;
            this.label9.Text = "Logs:";
            // 
            // check_cli_minimized
            // 
            this.check_cli_minimized.AutoSize = true;
            this.check_cli_minimized.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_cli_minimized.Location = new System.Drawing.Point(71, 18);
            this.check_cli_minimized.Name = "check_cli_minimized";
            this.check_cli_minimized.Size = new System.Drawing.Size(155, 17);
            this.check_cli_minimized.TabIndex = 76;
            this.check_cli_minimized.Text = "Use window minimized";
            this.ToolTip.SetToolTip(this.check_cli_minimized, "Starts a CLI window minimized.");
            this.check_cli_minimized.UseVisualStyleBackColor = true;
            this.check_cli_minimized.CheckedChanged += new System.EventHandler(this.check_cli_minimized_CheckedChanged);
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label12.Location = new System.Drawing.Point(21, 19);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(32, 13);
            this.label12.TabIndex = 75;
            this.label12.Text = "CLI:";
            // 
            // drp_Priority
            // 
            this.drp_Priority.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_Priority.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_Priority.FormattingEnabled = true;
            this.drp_Priority.Items.AddRange(new object[] {
            "Realtime",
            "High",
            "Above Normal",
            "Normal",
            "Below Normal",
            "Low"});
            this.drp_Priority.Location = new System.Drawing.Point(177, 72);
            this.drp_Priority.Name = "drp_Priority";
            this.drp_Priority.Size = new System.Drawing.Size(111, 21);
            this.drp_Priority.TabIndex = 43;
            this.ToolTip.SetToolTip(this.drp_Priority, "Set the application priority level for the CLI. \r\nIt\'s best to leave this on Belo" +
                    "w Normal if you wish to use your system whilst encoding with HandBrake.\r\n");
            this.drp_Priority.SelectedIndexChanged += new System.EventHandler(this.drp_Priority_SelectedIndexChanged);
            // 
            // Label11
            // 
            this.Label11.AutoSize = true;
            this.Label11.BackColor = System.Drawing.Color.Transparent;
            this.Label11.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label11.Location = new System.Drawing.Point(68, 48);
            this.Label11.Name = "Label11";
            this.Label11.Size = new System.Drawing.Size(103, 13);
            this.Label11.TabIndex = 40;
            this.Label11.Text = "Processor cores:";
            // 
            // drp_processors
            // 
            this.drp_processors.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_processors.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_processors.FormattingEnabled = true;
            this.drp_processors.Items.AddRange(new object[] {
            "Automatic",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8"});
            this.drp_processors.Location = new System.Drawing.Point(177, 45);
            this.drp_processors.Name = "drp_processors";
            this.drp_processors.Size = new System.Drawing.Size(111, 21);
            this.drp_processors.TabIndex = 41;
            this.ToolTip.SetToolTip(this.drp_processors, "The number of processor\'s / processor cores. Unless your having problems, leave o" +
                    "n Automatic.");
            this.drp_processors.SelectedIndexChanged += new System.EventHandler(this.drp_processors_SelectedIndexChanged);
            // 
            // Label4
            // 
            this.Label4.AutoSize = true;
            this.Label4.BackColor = System.Drawing.Color.Transparent;
            this.Label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label4.Location = new System.Drawing.Point(68, 75);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(84, 13);
            this.Label4.TabIndex = 42;
            this.Label4.Text = "Priority level:";
            // 
            // tab_advanced
            // 
            this.tab_advanced.Controls.Add(this.lbl_appcastUnstable);
            this.tab_advanced.Controls.Add(this.check_mainMinimize);
            this.tab_advanced.Controls.Add(this.check_snapshot);
            this.tab_advanced.Controls.Add(this.btn_drive_detect);
            this.tab_advanced.Controls.Add(this.label6);
            this.tab_advanced.Location = new System.Drawing.Point(4, 22);
            this.tab_advanced.Name = "tab_advanced";
            this.tab_advanced.Padding = new System.Windows.Forms.Padding(3);
            this.tab_advanced.Size = new System.Drawing.Size(482, 259);
            this.tab_advanced.TabIndex = 4;
            this.tab_advanced.Text = "Advanced / Other";
            this.tab_advanced.UseVisualStyleBackColor = true;
            // 
            // check_snapshot
            // 
            this.check_snapshot.AutoSize = true;
            this.check_snapshot.BackColor = System.Drawing.Color.Transparent;
            this.check_snapshot.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_snapshot.Location = new System.Drawing.Point(76, 81);
            this.check_snapshot.Name = "check_snapshot";
            this.check_snapshot.Size = new System.Drawing.Size(273, 17);
            this.check_snapshot.TabIndex = 80;
            this.check_snapshot.Text = "Check for unstable development snapshots";
            this.ToolTip.SetToolTip(this.check_snapshot, "Enables the built in update checker to check for the latest development snapshot " +
                    "builds.\r\nWarning: These are considered unstable builds and are not supported!");
            this.check_snapshot.UseVisualStyleBackColor = false;
            this.check_snapshot.CheckedChanged += new System.EventHandler(this.check_snapshot_CheckedChanged);
            // 
            // btn_drive_detect
            // 
            this.btn_drive_detect.AutoSize = true;
            this.btn_drive_detect.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_drive_detect.Location = new System.Drawing.Point(76, 18);
            this.btn_drive_detect.Name = "btn_drive_detect";
            this.btn_drive_detect.Size = new System.Drawing.Size(383, 17);
            this.btn_drive_detect.TabIndex = 72;
            this.btn_drive_detect.Text = "Enable Drive Detection in the \"Source\" button dropdown menu";
            this.ToolTip.SetToolTip(this.btn_drive_detect, "Enables the Source button\'s ability to detect DVD\'s.\r\nIf this option is enabled, " +
                    "DVD\'s will be listed in the source menu.");
            this.btn_drive_detect.UseVisualStyleBackColor = true;
            this.btn_drive_detect.CheckedChanged += new System.EventHandler(this.btn_drive_detect_CheckedChanged);
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(35, 19);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(35, 13);
            this.label6.TabIndex = 71;
            this.label6.Text = "GUI:";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(50, 24);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(131, 13);
            this.label8.TabIndex = 61;
            this.label8.Text = "HandBrake Options";
            // 
            // pictureBox2
            // 
            this.pictureBox2.Image = global::Handbrake.Properties.Resources.General_Preferences;
            this.pictureBox2.Location = new System.Drawing.Point(12, 12);
            this.pictureBox2.Name = "pictureBox2";
            this.pictureBox2.Size = new System.Drawing.Size(32, 32);
            this.pictureBox2.TabIndex = 60;
            this.pictureBox2.TabStop = false;
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 1000;
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
            // check_mainMinimize
            // 
            this.check_mainMinimize.AutoSize = true;
            this.check_mainMinimize.BackColor = System.Drawing.Color.Transparent;
            this.check_mainMinimize.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_mainMinimize.Location = new System.Drawing.Point(76, 41);
            this.check_mainMinimize.Name = "check_mainMinimize";
            this.check_mainMinimize.Size = new System.Drawing.Size(286, 17);
            this.check_mainMinimize.TabIndex = 82;
            this.check_mainMinimize.Text = "Minimize to System Tray rather than task bar";
            this.ToolTip.SetToolTip(this.check_mainMinimize, "Minimize the window to the system tray rather than the task bar.\r\nThe system tray" +
                    " icon has encode status notifications.\r\nNote: requires restart to take effect!\r\n" +
                    "");
            this.check_mainMinimize.UseVisualStyleBackColor = false;
            this.check_mainMinimize.CheckedChanged += new System.EventHandler(this.check_mainMinimize_CheckedChanged);
            // 
            // lbl_appcastUnstable
            // 
            this.lbl_appcastUnstable.AutoSize = true;
            this.lbl_appcastUnstable.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_appcastUnstable.Location = new System.Drawing.Point(6, 82);
            this.lbl_appcastUnstable.Name = "lbl_appcastUnstable";
            this.lbl_appcastUnstable.Size = new System.Drawing.Size(64, 13);
            this.lbl_appcastUnstable.TabIndex = 83;
            this.lbl_appcastUnstable.Text = "Updates:";
            // 
            // frmOptions
            // 
            this.ClientSize = new System.Drawing.Size(514, 375);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.pictureBox2);
            this.Controls.Add(this.tab_options);
            this.Controls.Add(this.btn_close);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmOptions";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Options";
            this.tab_options.ResumeLayout(false);
            this.tab_general.ResumeLayout(false);
            this.tab_general.PerformLayout();
            this.tab_picture.ResumeLayout(false);
            this.tab_picture.PerformLayout();
            this.tab_cli.ResumeLayout(false);
            this.tab_cli.PerformLayout();
            this.tab_advanced.ResumeLayout(false);
            this.tab_advanced.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

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
        internal System.Windows.Forms.ComboBox drp_Priority;
        internal System.Windows.Forms.Label Label11;
        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.ComboBox drp_processors;
        internal System.Windows.Forms.CheckBox check_tooltip;
        internal System.Windows.Forms.CheckBox check_updateCheck;
        internal System.Windows.Forms.CheckBox check_userDefaultSettings;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TabPage tab_advanced;
        internal System.Windows.Forms.CheckBox btn_drive_detect;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label13;
        internal System.Windows.Forms.Button btn_browse;
        internal System.Windows.Forms.Label label10;
        private System.Windows.Forms.TextBox text_an_path;
        internal System.Windows.Forms.CheckBox check_autoNaming;
        internal System.Windows.Forms.CheckBox check_cli_minimized;
        private System.Windows.Forms.Label label12;
        internal System.Windows.Forms.ToolTip ToolTip;
        internal System.Windows.Forms.CheckBox check_snapshot;
        private System.Windows.Forms.TabPage tab_picture;
        private System.Windows.Forms.TextBox txt_decomb;
        private System.Windows.Forms.Label label3;
        internal System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txt_autoNameFormat;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label9;
        internal System.Windows.Forms.Label label15;
        internal System.Windows.Forms.Button btn_saveLog;
        internal System.Windows.Forms.Label label14;
        private System.Windows.Forms.TextBox text_logPath;
        internal System.Windows.Forms.CheckBox check_keepLogs;
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
        private System.Windows.Forms.Label lbl_appcastUnstable;
        internal System.Windows.Forms.CheckBox check_mainMinimize;
    }
}