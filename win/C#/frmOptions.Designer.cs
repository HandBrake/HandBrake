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
            this.drp_Priority = new System.Windows.Forms.ComboBox();
            this.Label4 = new System.Windows.Forms.Label();
            this.check_verbose = new System.Windows.Forms.CheckBox();
            this.drp_processors = new System.Windows.Forms.ComboBox();
            this.File_Save = new System.Windows.Forms.SaveFileDialog();
            this.Label11 = new System.Windows.Forms.Label();
            this.check_guiDebug = new System.Windows.Forms.CheckBox();
            this.btn_close = new System.Windows.Forms.Button();
            this.drp_completeOption = new System.Windows.Forms.ComboBox();
            this.label5 = new System.Windows.Forms.Label();
            this.tab_options = new System.Windows.Forms.TabControl();
            this.tab_general = new System.Windows.Forms.TabPage();
            this.check_updatePresets = new System.Windows.Forms.CheckBox();
            this.check_showPreset = new System.Windows.Forms.CheckBox();
            this.check_tooltip = new System.Windows.Forms.CheckBox();
            this.check_updateCheck = new System.Windows.Forms.CheckBox();
            this.check_userDefaultSettings = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.tab_debug = new System.Windows.Forms.TabPage();
            this.label6 = new System.Windows.Forms.Label();
            this.tab_advanced = new System.Windows.Forms.TabPage();
            this.label3 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.pictureBox2 = new System.Windows.Forms.PictureBox();
            this.label7 = new System.Windows.Forms.Label();
            this.tab_options.SuspendLayout();
            this.tab_general.SuspendLayout();
            this.tab_debug.SuspendLayout();
            this.tab_advanced.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
            this.SuspendLayout();
            // 
            // drp_Priority
            // 
            this.drp_Priority.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.drp_Priority.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.drp_Priority.FormattingEnabled = true;
            this.drp_Priority.Items.AddRange(new object[] {
            "Realtime",
            "High",
            "Above Normal",
            "Normal",
            "Below Normal",
            "Low"});
            this.drp_Priority.Location = new System.Drawing.Point(161, 67);
            this.drp_Priority.Name = "drp_Priority";
            this.drp_Priority.Size = new System.Drawing.Size(111, 21);
            this.drp_Priority.TabIndex = 43;
            this.drp_Priority.SelectedIndexChanged += new System.EventHandler(this.drp_Priority_SelectedIndexChanged);
            // 
            // Label4
            // 
            this.Label4.AutoSize = true;
            this.Label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label4.Location = new System.Drawing.Point(13, 70);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(132, 13);
            this.Label4.TabIndex = 42;
            this.Label4.Text = "Default Priority Level:";
            // 
            // check_verbose
            // 
            this.check_verbose.AutoSize = true;
            this.check_verbose.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_verbose.Location = new System.Drawing.Point(16, 58);
            this.check_verbose.Name = "check_verbose";
            this.check_verbose.Size = new System.Drawing.Size(139, 17);
            this.check_verbose.TabIndex = 51;
            this.check_verbose.Text = "Enable Verbose CLI";
            this.check_verbose.UseVisualStyleBackColor = true;
            this.check_verbose.CheckedChanged += new System.EventHandler(this.check_verbose_CheckedChanged);
            // 
            // drp_processors
            // 
            this.drp_processors.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
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
            this.drp_processors.Location = new System.Drawing.Point(161, 40);
            this.drp_processors.Name = "drp_processors";
            this.drp_processors.Size = new System.Drawing.Size(111, 21);
            this.drp_processors.TabIndex = 41;
            this.drp_processors.SelectedIndexChanged += new System.EventHandler(this.drp_processors_SelectedIndexChanged);
            // 
            // File_Save
            // 
            this.File_Save.DefaultExt = "hb";
            this.File_Save.Filter = "txt|*.txt";
            // 
            // Label11
            // 
            this.Label11.AutoSize = true;
            this.Label11.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label11.Location = new System.Drawing.Point(13, 43);
            this.Label11.Name = "Label11";
            this.Label11.Size = new System.Drawing.Size(142, 13);
            this.Label11.TabIndex = 40;
            this.Label11.Text = "Number of processors: ";
            // 
            // check_guiDebug
            // 
            this.check_guiDebug.AutoSize = true;
            this.check_guiDebug.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_guiDebug.Location = new System.Drawing.Point(16, 35);
            this.check_guiDebug.Name = "check_guiDebug";
            this.check_guiDebug.Size = new System.Drawing.Size(131, 17);
            this.check_guiDebug.TabIndex = 52;
            this.check_guiDebug.Text = "Enable GUI Debug";
            this.check_guiDebug.UseVisualStyleBackColor = true;
            this.check_guiDebug.CheckedChanged += new System.EventHandler(this.check_guiDebug_CheckedChanged);
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(288, 297);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(107, 22);
            this.btn_close.TabIndex = 53;
            this.btn_close.Text = "Save && Close";
            this.btn_close.UseVisualStyleBackColor = false;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // drp_completeOption
            // 
            this.drp_completeOption.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
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
            this.drp_completeOption.Location = new System.Drawing.Point(192, 170);
            this.drp_completeOption.Name = "drp_completeOption";
            this.drp_completeOption.Size = new System.Drawing.Size(111, 21);
            this.drp_completeOption.TabIndex = 43;
            this.drp_completeOption.SelectedIndexChanged += new System.EventHandler(this.drp_completeOption_SelectedIndexChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(13, 173);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(173, 13);
            this.label5.TabIndex = 42;
            this.label5.Text = "When the encode completes:";
            // 
            // tab_options
            // 
            this.tab_options.Controls.Add(this.tab_general);
            this.tab_options.Controls.Add(this.tab_debug);
            this.tab_options.Controls.Add(this.tab_advanced);
            this.tab_options.Location = new System.Drawing.Point(12, 63);
            this.tab_options.Name = "tab_options";
            this.tab_options.SelectedIndex = 0;
            this.tab_options.Size = new System.Drawing.Size(382, 228);
            this.tab_options.TabIndex = 58;
            // 
            // tab_general
            // 
            this.tab_general.BackColor = System.Drawing.SystemColors.ControlLight;
            this.tab_general.Controls.Add(this.check_updatePresets);
            this.tab_general.Controls.Add(this.drp_completeOption);
            this.tab_general.Controls.Add(this.label5);
            this.tab_general.Controls.Add(this.check_showPreset);
            this.tab_general.Controls.Add(this.check_tooltip);
            this.tab_general.Controls.Add(this.check_updateCheck);
            this.tab_general.Controls.Add(this.check_userDefaultSettings);
            this.tab_general.Controls.Add(this.label2);
            this.tab_general.Controls.Add(this.label1);
            this.tab_general.Location = new System.Drawing.Point(4, 22);
            this.tab_general.Name = "tab_general";
            this.tab_general.Padding = new System.Windows.Forms.Padding(3);
            this.tab_general.Size = new System.Drawing.Size(374, 202);
            this.tab_general.TabIndex = 0;
            this.tab_general.Text = "General";
            // 
            // check_updatePresets
            // 
            this.check_updatePresets.AutoSize = true;
            this.check_updatePresets.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_updatePresets.Location = new System.Drawing.Point(16, 125);
            this.check_updatePresets.Name = "check_updatePresets";
            this.check_updatePresets.Size = new System.Drawing.Size(112, 17);
            this.check_updatePresets.TabIndex = 59;
            this.check_updatePresets.Text = "Update presets";
            this.check_updatePresets.UseVisualStyleBackColor = true;
            this.check_updatePresets.CheckedChanged += new System.EventHandler(this.check_updatePresets_CheckedChanged);
            // 
            // check_showPreset
            // 
            this.check_showPreset.AutoSize = true;
            this.check_showPreset.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_showPreset.Location = new System.Drawing.Point(16, 56);
            this.check_showPreset.Name = "check_showPreset";
            this.check_showPreset.Size = new System.Drawing.Size(136, 17);
            this.check_showPreset.TabIndex = 58;
            this.check_showPreset.Text = "Hide the preset bar";
            this.check_showPreset.UseVisualStyleBackColor = true;
            this.check_showPreset.CheckedChanged += new System.EventHandler(this.check_showPreset_CheckedChanged);
            // 
            // check_tooltip
            // 
            this.check_tooltip.AutoSize = true;
            this.check_tooltip.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_tooltip.Location = new System.Drawing.Point(16, 79);
            this.check_tooltip.Name = "check_tooltip";
            this.check_tooltip.Size = new System.Drawing.Size(135, 17);
            this.check_tooltip.TabIndex = 57;
            this.check_tooltip.Text = "Enable GUI tooltips";
            this.check_tooltip.UseVisualStyleBackColor = true;
            this.check_tooltip.CheckedChanged += new System.EventHandler(this.check_tooltip_CheckedChanged);
            // 
            // check_updateCheck
            // 
            this.check_updateCheck.AutoSize = true;
            this.check_updateCheck.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_updateCheck.Location = new System.Drawing.Point(16, 33);
            this.check_updateCheck.Name = "check_updateCheck";
            this.check_updateCheck.Size = new System.Drawing.Size(131, 17);
            this.check_updateCheck.TabIndex = 55;
            this.check_updateCheck.Text = "Check for updates";
            this.check_updateCheck.UseVisualStyleBackColor = true;
            this.check_updateCheck.CheckedChanged += new System.EventHandler(this.check_updateCheck_CheckedChanged);
            // 
            // check_userDefaultSettings
            // 
            this.check_userDefaultSettings.AutoSize = true;
            this.check_userDefaultSettings.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_userDefaultSettings.Location = new System.Drawing.Point(16, 102);
            this.check_userDefaultSettings.Name = "check_userDefaultSettings";
            this.check_userDefaultSettings.Size = new System.Drawing.Size(166, 17);
            this.check_userDefaultSettings.TabIndex = 56;
            this.check_userDefaultSettings.Text = "Load my default settings";
            this.check_userDefaultSettings.UseVisualStyleBackColor = true;
            this.check_userDefaultSettings.CheckedChanged += new System.EventHandler(this.check_userDefaultSettings_CheckedChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(13, 156);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(80, 13);
            this.label2.TabIndex = 54;
            this.label2.Text = "When Done";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(13, 13);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(76, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "On Startup";
            // 
            // tab_debug
            // 
            this.tab_debug.BackColor = System.Drawing.SystemColors.ControlLight;
            this.tab_debug.Controls.Add(this.check_verbose);
            this.tab_debug.Controls.Add(this.check_guiDebug);
            this.tab_debug.Controls.Add(this.label6);
            this.tab_debug.Location = new System.Drawing.Point(4, 22);
            this.tab_debug.Name = "tab_debug";
            this.tab_debug.Size = new System.Drawing.Size(374, 202);
            this.tab_debug.TabIndex = 2;
            this.tab_debug.Text = "Debug Options";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(13, 13);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(173, 13);
            this.label6.TabIndex = 1;
            this.label6.Text = "Verbose and Debug mode";
            // 
            // tab_advanced
            // 
            this.tab_advanced.BackColor = System.Drawing.SystemColors.ControlLight;
            this.tab_advanced.Controls.Add(this.label3);
            this.tab_advanced.Controls.Add(this.drp_Priority);
            this.tab_advanced.Controls.Add(this.Label11);
            this.tab_advanced.Controls.Add(this.Label4);
            this.tab_advanced.Controls.Add(this.drp_processors);
            this.tab_advanced.Location = new System.Drawing.Point(4, 22);
            this.tab_advanced.Name = "tab_advanced";
            this.tab_advanced.Padding = new System.Windows.Forms.Padding(3);
            this.tab_advanced.Size = new System.Drawing.Size(374, 202);
            this.tab_advanced.TabIndex = 1;
            this.tab_advanced.Text = "Advanced";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(13, 13);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(176, 13);
            this.label3.TabIndex = 44;
            this.label3.Text = "Process(or) Configuration";
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
            // frmOptions
            // 
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(407, 328);
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
            this.tab_debug.ResumeLayout(false);
            this.tab_debug.PerformLayout();
            this.tab_advanced.ResumeLayout(false);
            this.tab_advanced.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.ComboBox drp_Priority;
        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.CheckBox check_verbose;
        internal System.Windows.Forms.ComboBox drp_processors;
        internal System.Windows.Forms.SaveFileDialog File_Save;
        internal System.Windows.Forms.Label Label11;
        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.CheckBox check_guiDebug;
        internal System.Windows.Forms.ComboBox drp_completeOption;
        internal System.Windows.Forms.Label label5;
        private System.Windows.Forms.TabControl tab_options;
        private System.Windows.Forms.TabPage tab_general;
        private System.Windows.Forms.TabPage tab_advanced;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        internal System.Windows.Forms.CheckBox check_showPreset;
        internal System.Windows.Forms.CheckBox check_tooltip;
        internal System.Windows.Forms.CheckBox check_updateCheck;
        internal System.Windows.Forms.CheckBox check_userDefaultSettings;
        private System.Windows.Forms.TabPage tab_debug;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.PictureBox pictureBox2;
        private System.Windows.Forms.Label label7;
        internal System.Windows.Forms.CheckBox check_updatePresets;
    }
}