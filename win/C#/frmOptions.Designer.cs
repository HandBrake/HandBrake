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
            this.GroupBox2 = new System.Windows.Forms.GroupBox();
            this.check_tooltip = new System.Windows.Forms.CheckBox();
            this.check_updateCheck = new System.Windows.Forms.CheckBox();
            this.check_userDefaultSettings = new System.Windows.Forms.CheckBox();
            this.Label4 = new System.Windows.Forms.Label();
            this.check_verbose = new System.Windows.Forms.CheckBox();
            this.drp_processors = new System.Windows.Forms.ComboBox();
            this.File_Save = new System.Windows.Forms.SaveFileDialog();
            this.Label11 = new System.Windows.Forms.Label();
            this.GroupBox3 = new System.Windows.Forms.GroupBox();
            this.GroupBox1 = new System.Windows.Forms.GroupBox();
            this.check_guiDebug = new System.Windows.Forms.CheckBox();
            this.btn_close = new System.Windows.Forms.Button();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.drp_completeOption = new System.Windows.Forms.ComboBox();
            this.label5 = new System.Windows.Forms.Label();
            this.check_showPreset = new System.Windows.Forms.CheckBox();
            this.GroupBox2.SuspendLayout();
            this.GroupBox3.SuspendLayout();
            this.GroupBox1.SuspendLayout();
            this.groupBox4.SuspendLayout();
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
            this.drp_Priority.Location = new System.Drawing.Point(223, 58);
            this.drp_Priority.Name = "drp_Priority";
            this.drp_Priority.Size = new System.Drawing.Size(111, 21);
            this.drp_Priority.TabIndex = 43;
            this.drp_Priority.SelectedIndexChanged += new System.EventHandler(this.drp_Priority_SelectedIndexChanged);
            // 
            // GroupBox2
            // 
            this.GroupBox2.BackColor = System.Drawing.SystemColors.ControlLight;
            this.GroupBox2.Controls.Add(this.check_showPreset);
            this.GroupBox2.Controls.Add(this.check_tooltip);
            this.GroupBox2.Controls.Add(this.check_updateCheck);
            this.GroupBox2.Controls.Add(this.check_userDefaultSettings);
            this.GroupBox2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GroupBox2.Location = new System.Drawing.Point(12, 11);
            this.GroupBox2.Name = "GroupBox2";
            this.GroupBox2.Size = new System.Drawing.Size(386, 123);
            this.GroupBox2.TabIndex = 55;
            this.GroupBox2.TabStop = false;
            this.GroupBox2.Text = "General Settings";
            // 
            // check_tooltip
            // 
            this.check_tooltip.AutoSize = true;
            this.check_tooltip.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_tooltip.Location = new System.Drawing.Point(15, 71);
            this.check_tooltip.Name = "check_tooltip";
            this.check_tooltip.Size = new System.Drawing.Size(135, 17);
            this.check_tooltip.TabIndex = 49;
            this.check_tooltip.Text = "Enable GUI tooltips";
            this.check_tooltip.UseVisualStyleBackColor = true;
            this.check_tooltip.CheckedChanged += new System.EventHandler(this.check_tooltip_CheckedChanged);
            // 
            // check_updateCheck
            // 
            this.check_updateCheck.AutoSize = true;
            this.check_updateCheck.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_updateCheck.Location = new System.Drawing.Point(15, 25);
            this.check_updateCheck.Name = "check_updateCheck";
            this.check_updateCheck.Size = new System.Drawing.Size(170, 17);
            this.check_updateCheck.TabIndex = 1;
            this.check_updateCheck.Text = "Update Check on Startup";
            this.check_updateCheck.UseVisualStyleBackColor = true;
            this.check_updateCheck.CheckedChanged += new System.EventHandler(this.check_updateCheck_CheckedChanged);
            // 
            // check_userDefaultSettings
            // 
            this.check_userDefaultSettings.AutoSize = true;
            this.check_userDefaultSettings.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_userDefaultSettings.Location = new System.Drawing.Point(15, 48);
            this.check_userDefaultSettings.Name = "check_userDefaultSettings";
            this.check_userDefaultSettings.Size = new System.Drawing.Size(248, 17);
            this.check_userDefaultSettings.TabIndex = 47;
            this.check_userDefaultSettings.Text = "Load Users Default Settings on Startup";
            this.check_userDefaultSettings.UseVisualStyleBackColor = true;
            this.check_userDefaultSettings.CheckedChanged += new System.EventHandler(this.check_userDefaultSettings_CheckedChanged);
            // 
            // Label4
            // 
            this.Label4.AutoSize = true;
            this.Label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label4.Location = new System.Drawing.Point(13, 61);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(132, 13);
            this.Label4.TabIndex = 42;
            this.Label4.Text = "Default Priority Level:";
            // 
            // check_verbose
            // 
            this.check_verbose.AutoSize = true;
            this.check_verbose.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_verbose.Location = new System.Drawing.Point(20, 31);
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
            this.drp_processors.Location = new System.Drawing.Point(223, 22);
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
            this.Label11.Location = new System.Drawing.Point(12, 25);
            this.Label11.Name = "Label11";
            this.Label11.Size = new System.Drawing.Size(142, 13);
            this.Label11.TabIndex = 40;
            this.Label11.Text = "Number of processors: ";
            // 
            // GroupBox3
            // 
            this.GroupBox3.BackColor = System.Drawing.SystemColors.ControlLight;
            this.GroupBox3.Controls.Add(this.drp_Priority);
            this.GroupBox3.Controls.Add(this.Label4);
            this.GroupBox3.Controls.Add(this.drp_processors);
            this.GroupBox3.Controls.Add(this.Label11);
            this.GroupBox3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GroupBox3.Location = new System.Drawing.Point(12, 211);
            this.GroupBox3.Name = "GroupBox3";
            this.GroupBox3.Size = new System.Drawing.Size(386, 94);
            this.GroupBox3.TabIndex = 56;
            this.GroupBox3.TabStop = false;
            this.GroupBox3.Text = "Processor Detection";
            // 
            // GroupBox1
            // 
            this.GroupBox1.BackColor = System.Drawing.SystemColors.ControlLight;
            this.GroupBox1.Controls.Add(this.check_guiDebug);
            this.GroupBox1.Controls.Add(this.check_verbose);
            this.GroupBox1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.GroupBox1.Location = new System.Drawing.Point(12, 311);
            this.GroupBox1.Name = "GroupBox1";
            this.GroupBox1.Size = new System.Drawing.Size(386, 70);
            this.GroupBox1.TabIndex = 54;
            this.GroupBox1.TabStop = false;
            this.GroupBox1.Text = "Verbose && Debug Mode";
            // 
            // check_guiDebug
            // 
            this.check_guiDebug.AutoSize = true;
            this.check_guiDebug.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_guiDebug.Location = new System.Drawing.Point(223, 31);
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
            this.btn_close.Location = new System.Drawing.Point(291, 391);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(107, 22);
            this.btn_close.TabIndex = 53;
            this.btn_close.Text = "Close";
            this.btn_close.UseVisualStyleBackColor = false;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.drp_completeOption);
            this.groupBox4.Controls.Add(this.label5);
            this.groupBox4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.groupBox4.Location = new System.Drawing.Point(12, 140);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(386, 65);
            this.groupBox4.TabIndex = 57;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "On Completion Options";
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
            this.drp_completeOption.Location = new System.Drawing.Point(223, 29);
            this.drp_completeOption.Name = "drp_completeOption";
            this.drp_completeOption.Size = new System.Drawing.Size(111, 21);
            this.drp_completeOption.TabIndex = 43;
            this.drp_completeOption.SelectedIndexChanged += new System.EventHandler(this.drp_completeOption_SelectedIndexChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(13, 29);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(173, 13);
            this.label5.TabIndex = 42;
            this.label5.Text = "When the encode completes:";
            // 
            // check_showPreset
            // 
            this.check_showPreset.AutoSize = true;
            this.check_showPreset.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.check_showPreset.Location = new System.Drawing.Point(15, 94);
            this.check_showPreset.Name = "check_showPreset";
            this.check_showPreset.Size = new System.Drawing.Size(362, 17);
            this.check_showPreset.TabIndex = 50;
            this.check_showPreset.Text = "Always Show Presets (Hide\'s the Show/Hide preset button)";
            this.check_showPreset.UseVisualStyleBackColor = true;
            this.check_showPreset.CheckedChanged += new System.EventHandler(this.check_showPreset_CheckedChanged);
            // 
            // frmOptions
            // 
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(413, 422);
            this.Controls.Add(this.groupBox4);
            this.Controls.Add(this.GroupBox2);
            this.Controls.Add(this.GroupBox3);
            this.Controls.Add(this.GroupBox1);
            this.Controls.Add(this.btn_close);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmOptions";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Options";
            this.GroupBox2.ResumeLayout(false);
            this.GroupBox2.PerformLayout();
            this.GroupBox3.ResumeLayout(false);
            this.GroupBox3.PerformLayout();
            this.GroupBox1.ResumeLayout(false);
            this.GroupBox1.PerformLayout();
            this.groupBox4.ResumeLayout(false);
            this.groupBox4.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        internal System.Windows.Forms.ComboBox drp_Priority;
        internal System.Windows.Forms.GroupBox GroupBox2;
        internal System.Windows.Forms.CheckBox check_updateCheck;
        internal System.Windows.Forms.CheckBox check_userDefaultSettings;
        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.CheckBox check_verbose;
        internal System.Windows.Forms.ComboBox drp_processors;
        internal System.Windows.Forms.SaveFileDialog File_Save;
        internal System.Windows.Forms.Label Label11;
        internal System.Windows.Forms.GroupBox GroupBox3;
        internal System.Windows.Forms.GroupBox GroupBox1;
        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.CheckBox check_guiDebug;
        internal System.Windows.Forms.CheckBox check_tooltip;
        private System.Windows.Forms.GroupBox groupBox4;
        internal System.Windows.Forms.ComboBox drp_completeOption;
        internal System.Windows.Forms.Label label5;
        internal System.Windows.Forms.CheckBox check_showPreset;
    }
}