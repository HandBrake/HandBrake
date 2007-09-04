namespace Handbrake
{
    partial class frmAbout
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmAbout));
            this.Label4 = new System.Windows.Forms.Label();
            this.btn_close = new System.Windows.Forms.Button();
            this.Version = new System.Windows.Forms.Label();
            this.Label3 = new System.Windows.Forms.Label();
            this.Label2 = new System.Windows.Forms.Label();
            this.PictureBox1 = new System.Windows.Forms.PictureBox();
            this.Label1 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.lbl_guiVer = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // Label4
            // 
            this.Label4.AutoSize = true;
            this.Label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label4.Location = new System.Drawing.Point(144, 58);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(375, 104);
            this.Label4.TabIndex = 28;
            this.Label4.Text = resources.GetString("Label4.Text");
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_close.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(415, 248);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(99, 22);
            this.btn_close.TabIndex = 27;
            this.btn_close.Text = "Close";
            this.btn_close.UseVisualStyleBackColor = false;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // Version
            // 
            this.Version.AutoSize = true;
            this.Version.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Version.Location = new System.Drawing.Point(228, 13);
            this.Version.Name = "Version";
            this.Version.Size = new System.Drawing.Size(72, 13);
            this.Version.TabIndex = 26;
            this.Version.Text = "{Version}";
            // 
            // Label3
            // 
            this.Label3.AutoSize = true;
            this.Label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label3.Location = new System.Drawing.Point(144, 13);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(78, 13);
            this.Label3.TabIndex = 25;
            this.Label3.Text = "Handbrake";
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(144, 172);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(93, 13);
            this.Label2.TabIndex = 24;
            this.Label2.Text = ":::: Credits ::::";
            // 
            // PictureBox1
            // 
            this.PictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("PictureBox1.Image")));
            this.PictureBox1.InitialImage = null;
            this.PictureBox1.Location = new System.Drawing.Point(7, 8);
            this.PictureBox1.Name = "PictureBox1";
            this.PictureBox1.Size = new System.Drawing.Size(131, 132);
            this.PictureBox1.TabIndex = 23;
            this.PictureBox1.TabStop = false;
            // 
            // Label1
            // 
            this.Label1.AutoSize = true;
            this.Label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label1.Location = new System.Drawing.Point(144, 195);
            this.Label1.Name = "Label1";
            this.Label1.Size = new System.Drawing.Size(363, 39);
            this.Label1.TabIndex = 22;
            this.Label1.Text = "All credits and GPL licence files are located in the \"doc\" folder.\r\nYou can acces" +
                "s this file via the \"doc\" shortcut located in\r\nHandBrake\'s start menu folder.";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(144, 30);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(66, 13);
            this.label5.TabIndex = 29;
            this.label5.Text = "GUI Build:";
            // 
            // lbl_guiVer
            // 
            this.lbl_guiVer.AutoSize = true;
            this.lbl_guiVer.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_guiVer.Location = new System.Drawing.Point(228, 30);
            this.lbl_guiVer.Name = "lbl_guiVer";
            this.lbl_guiVer.Size = new System.Drawing.Size(64, 13);
            this.lbl_guiVer.TabIndex = 30;
            this.lbl_guiVer.Text = "{Version}";
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(147, 45);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(372, 10);
            this.groupBox1.TabIndex = 31;
            this.groupBox1.TabStop = false;
            // 
            // frmAbout
            // 
            this.AcceptButton = this.btn_close;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(526, 278);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.lbl_guiVer);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.Label4);
            this.Controls.Add(this.btn_close);
            this.Controls.Add(this.Version);
            this.Controls.Add(this.Label3);
            this.Controls.Add(this.Label2);
            this.Controls.Add(this.PictureBox1);
            this.Controls.Add(this.Label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(532, 284);
            this.Name = "frmAbout";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "About Handbrake";
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.Label Version;
        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.Label Label2;
        internal System.Windows.Forms.PictureBox PictureBox1;
        internal System.Windows.Forms.Label Label1;
        internal System.Windows.Forms.Label label5;
        internal System.Windows.Forms.Label lbl_guiVer;
        private System.Windows.Forms.GroupBox groupBox1;
    }
}