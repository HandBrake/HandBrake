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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmOptions));
            this.btn_close = new System.Windows.Forms.Button();
            this.drp_completeOption = new System.Windows.Forms.ComboBox();
            this.label5 = new System.Windows.Forms.Label();
            this.tab_options = new System.Windows.Forms.TabControl();
            this.tab_general = new System.Windows.Forms.TabPage();
            this.check_tooltip = new System.Windows.Forms.CheckBox();
            this.check_updateCheck = new System.Windows.Forms.CheckBox();
            this.check_userDefaultSettings = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.btn_browse = new System.Windows.Forms.Button();
            this.label10 = new System.Windows.Forms.Label();
            this.text_an_path = new System.Windows.Forms.TextBox();
            this.check_autoNaming = new System.Windows.Forms.CheckBox();
            this.label9 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.tab_debug = new System.Windows.Forms.TabPage();
            this.label3 = new System.Windows.Forms.Label();
            this.drp_Priority = new System.Windows.Forms.ComboBox();
            this.Label11 = new System.Windows.Forms.Label();
            this.drp_processors = new System.Windows.Forms.ComboBox();
            this.Label4 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.pictureBox2 = new System.Windows.Forms.PictureBox();
            this.label7 = new System.Windows.Forms.Label();
            this.pathFinder = new System.Windows.Forms.FolderBrowserDialog();
            this.label6 = new System.Windows.Forms.Label();
            this.btn_drive_detect = new System.Windows.Forms.CheckBox();
            this.tab_options.SuspendLayout();
            this.tab_general.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.tab_debug.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
            this.SuspendLayout();
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(324, 327);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(107, 22);
            this.btn_close.TabIndex = 53;
            this.btn_close.Text = "Save && Close";
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
            this.drp_completeOption.Location = new System.Drawing.Point(193, 39);
            this.drp_completeOption.Name = "drp_completeOption";
            this.drp_completeOption.Size = new System.Drawing.Size(111, 21);
            this.drp_completeOption.TabIndex = 43;
            this.drp_completeOption.SelectedIndexChanged += new System.EventHandler(this.drp_completeOption_SelectedIndexChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(14, 42);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(173, 13);
            this.label5.TabIndex = 42;
            this.label5.Text = "When the encode completes:";
            // 
            // tab_options
            // 
            this.tab_options.Controls.Add(this.tab_general);
            this.tab_options.Controls.Add(this.tabPage1);
            this.tab_options.Controls.Add(this.tab_debug);
            this.tab_options.Location = new System.Drawing.Point(12, 63);
            this.tab_options.Name = "tab_options";
            this.tab_options.SelectedIndex = 0;
            this.tab_options.Size = new System.Drawing.Size(419, 258);
            this.tab_options.TabIndex = 58;
            // 
            // tab_general
            // 
            this.tab_general.Controls.Add(this.check_tooltip);
            this.tab_general.Controls.Add(this.check_updateCheck);
            this.tab_general.Controls.Add(this.check_userDefaultSettings);
            this.tab_general.Controls.Add(this.label1);
            this.tab_general.Location = new System.Drawing.Point(4, 22);
            this.tab_general.Name = "tab_general";
            this.tab_general.Padding = new System.Windows.Forms.Padding(3);
            this.tab_general.Size = new System.Drawing.Size(411, 210);
            this.tab_general.TabIndex = 0;
            this.tab_general.Text = "Startup";
            this.tab_general.UseVisualStyleBackColor = true;
            // 
            // check_tooltip
            // 
            this.check_tooltip.AutoSize = true;
            this.check_tooltip.BackColor = System.Drawing.Color.Transparent;
            this.check_tooltip.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_tooltip.Location = new System.Drawing.Point(16, 56);
            this.check_tooltip.Name = "check_tooltip";
            this.check_tooltip.Size = new System.Drawing.Size(135, 17);
            this.check_tooltip.TabIndex = 57;
            this.check_tooltip.Text = "Enable GUI tooltips";
            this.check_tooltip.UseVisualStyleBackColor = false;
            this.check_tooltip.CheckedChanged += new System.EventHandler(this.check_tooltip_CheckedChanged);
            // 
            // check_updateCheck
            // 
            this.check_updateCheck.AutoSize = true;
            this.check_updateCheck.BackColor = System.Drawing.Color.Transparent;
            this.check_updateCheck.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_updateCheck.Location = new System.Drawing.Point(16, 33);
            this.check_updateCheck.Name = "check_updateCheck";
            this.check_updateCheck.Size = new System.Drawing.Size(131, 17);
            this.check_updateCheck.TabIndex = 55;
            this.check_updateCheck.Text = "Check for updates";
            this.check_updateCheck.UseVisualStyleBackColor = false;
            this.check_updateCheck.CheckedChanged += new System.EventHandler(this.check_updateCheck_CheckedChanged);
            // 
            // check_userDefaultSettings
            // 
            this.check_userDefaultSettings.AutoSize = true;
            this.check_userDefaultSettings.BackColor = System.Drawing.Color.Transparent;
            this.check_userDefaultSettings.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_userDefaultSettings.Location = new System.Drawing.Point(16, 79);
            this.check_userDefaultSettings.Name = "check_userDefaultSettings";
            this.check_userDefaultSettings.Size = new System.Drawing.Size(166, 17);
            this.check_userDefaultSettings.TabIndex = 56;
            this.check_userDefaultSettings.Text = "Load my default settings";
            this.check_userDefaultSettings.UseVisualStyleBackColor = false;
            this.check_userDefaultSettings.CheckedChanged += new System.EventHandler(this.check_userDefaultSettings_CheckedChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(13, 13);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(76, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "On Startup";
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.btn_drive_detect);
            this.tabPage1.Controls.Add(this.label6);
            this.tabPage1.Controls.Add(this.btn_browse);
            this.tabPage1.Controls.Add(this.label10);
            this.tabPage1.Controls.Add(this.text_an_path);
            this.tabPage1.Controls.Add(this.check_autoNaming);
            this.tabPage1.Controls.Add(this.label9);
            this.tabPage1.Controls.Add(this.label2);
            this.tabPage1.Controls.Add(this.drp_completeOption);
            this.tabPage1.Controls.Add(this.label5);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Size = new System.Drawing.Size(411, 232);
            this.tabPage1.TabIndex = 3;
            this.tabPage1.Text = "General";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // btn_browse
            // 
            this.btn_browse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_browse.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_browse.Location = new System.Drawing.Point(326, 146);
            this.btn_browse.Name = "btn_browse";
            this.btn_browse.Size = new System.Drawing.Size(68, 22);
            this.btn_browse.TabIndex = 62;
            this.btn_browse.Text = "Browse";
            this.btn_browse.UseVisualStyleBackColor = true;
            this.btn_browse.Click += new System.EventHandler(this.btn_browse_Click);
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label10.Location = new System.Drawing.Point(14, 131);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(145, 13);
            this.label10.TabIndex = 61;
            this.label10.Text = "Default AutoName Path:";
            // 
            // text_an_path
            // 
            this.text_an_path.Location = new System.Drawing.Point(17, 147);
            this.text_an_path.Name = "text_an_path";
            this.text_an_path.Size = new System.Drawing.Size(303, 21);
            this.text_an_path.TabIndex = 60;
            this.text_an_path.TextChanged += new System.EventHandler(this.text_an_path_TextChanged);
            // 
            // check_autoNaming
            // 
            this.check_autoNaming.AutoSize = true;
            this.check_autoNaming.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_autoNaming.Location = new System.Drawing.Point(17, 101);
            this.check_autoNaming.Name = "check_autoNaming";
            this.check_autoNaming.Size = new System.Drawing.Size(340, 17);
            this.check_autoNaming.TabIndex = 56;
            this.check_autoNaming.Text = "Use Auto Naming (uses source name and title number)";
            this.check_autoNaming.UseVisualStyleBackColor = true;
            this.check_autoNaming.CheckedChanged += new System.EventHandler(this.check_autoNaming_CheckedChanged);
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label9.Location = new System.Drawing.Point(14, 78);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(90, 13);
            this.label9.TabIndex = 55;
            this.label9.Text = "Auto Naming";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(14, 19);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(80, 13);
            this.label2.TabIndex = 54;
            this.label2.Text = "When Done";
            // 
            // tab_debug
            // 
            this.tab_debug.Controls.Add(this.label3);
            this.tab_debug.Controls.Add(this.drp_Priority);
            this.tab_debug.Controls.Add(this.Label11);
            this.tab_debug.Controls.Add(this.drp_processors);
            this.tab_debug.Controls.Add(this.Label4);
            this.tab_debug.Location = new System.Drawing.Point(4, 22);
            this.tab_debug.Name = "tab_debug";
            this.tab_debug.Size = new System.Drawing.Size(411, 210);
            this.tab_debug.TabIndex = 2;
            this.tab_debug.Text = "Performance";
            this.tab_debug.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.BackColor = System.Drawing.Color.Transparent;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(13, 13);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(176, 13);
            this.label3.TabIndex = 44;
            this.label3.Text = "Process(or) Configuration";
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
            this.drp_Priority.Location = new System.Drawing.Point(163, 68);
            this.drp_Priority.Name = "drp_Priority";
            this.drp_Priority.Size = new System.Drawing.Size(111, 21);
            this.drp_Priority.TabIndex = 43;
            this.drp_Priority.SelectedIndexChanged += new System.EventHandler(this.drp_Priority_SelectedIndexChanged);
            // 
            // Label11
            // 
            this.Label11.AutoSize = true;
            this.Label11.BackColor = System.Drawing.Color.Transparent;
            this.Label11.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label11.Location = new System.Drawing.Point(15, 44);
            this.Label11.Name = "Label11";
            this.Label11.Size = new System.Drawing.Size(142, 13);
            this.Label11.TabIndex = 40;
            this.Label11.Text = "Number of processors: ";
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
            this.drp_processors.Location = new System.Drawing.Point(163, 41);
            this.drp_processors.Name = "drp_processors";
            this.drp_processors.Size = new System.Drawing.Size(111, 21);
            this.drp_processors.TabIndex = 41;
            this.drp_processors.SelectedIndexChanged += new System.EventHandler(this.drp_processors_SelectedIndexChanged);
            // 
            // Label4
            // 
            this.Label4.AutoSize = true;
            this.Label4.BackColor = System.Drawing.Color.Transparent;
            this.Label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label4.Location = new System.Drawing.Point(15, 71);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(132, 13);
            this.Label4.TabIndex = 42;
            this.Label4.Text = "Default Priority Level:";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(50, 24);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(131, 13);
            this.label8.TabIndex = 61;
            this.label8.Text = "Handbrake Options";
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
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(50, 39);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(146, 13);
            this.label7.TabIndex = 59;
            this.label7.Text = "Modify program options.";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(14, 182);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(108, 13);
            this.label6.TabIndex = 63;
            this.label6.Text = "Drive Detection";
            // 
            // btn_drive_detect
            // 
            this.btn_drive_detect.AutoSize = true;
            this.btn_drive_detect.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_drive_detect.Location = new System.Drawing.Point(17, 204);
            this.btn_drive_detect.Name = "btn_drive_detect";
            this.btn_drive_detect.Size = new System.Drawing.Size(383, 17);
            this.btn_drive_detect.TabIndex = 64;
            this.btn_drive_detect.Text = "Enable Drive Detection in the \"Source\" button dropdown menu";
            this.btn_drive_detect.UseVisualStyleBackColor = true;
            this.btn_drive_detect.CheckedChanged += new System.EventHandler(this.btn_drive_detect_CheckedChanged);
            // 
            // frmOptions
            // 
            this.ClientSize = new System.Drawing.Size(443, 357);
            this.Controls.Add(this.label7);
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
            this.tabPage1.ResumeLayout(false);
            this.tabPage1.PerformLayout();
            this.tab_debug.ResumeLayout(false);
            this.tab_debug.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.ComboBox drp_completeOption;
        internal System.Windows.Forms.Label label5;
        private System.Windows.Forms.TabControl tab_options;
        private System.Windows.Forms.TabPage tab_general;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        internal System.Windows.Forms.CheckBox check_tooltip;
        internal System.Windows.Forms.CheckBox check_updateCheck;
        internal System.Windows.Forms.CheckBox check_userDefaultSettings;
        private System.Windows.Forms.TabPage tab_debug;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.PictureBox pictureBox2;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.Label label9;
        internal System.Windows.Forms.CheckBox check_autoNaming;
        private System.Windows.Forms.TextBox text_an_path;
        internal System.Windows.Forms.Button btn_browse;
        internal System.Windows.Forms.Label label10;
        private System.Windows.Forms.FolderBrowserDialog pathFinder;
        private System.Windows.Forms.Label label3;
        internal System.Windows.Forms.ComboBox drp_Priority;
        internal System.Windows.Forms.Label Label11;
        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.ComboBox drp_processors;
        internal System.Windows.Forms.CheckBox btn_drive_detect;
        private System.Windows.Forms.Label label6;
    }
}