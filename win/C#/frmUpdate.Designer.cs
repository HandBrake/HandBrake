namespace Handbrake
{
    partial class frmUpdate
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmUpdate));
            this.cliVersion = new System.Windows.Forms.Label();
            this.lbl_cliVersion = new System.Windows.Forms.Label();
            this.Label6 = new System.Windows.Forms.Label();
            this.Label7 = new System.Windows.Forms.Label();
            this.Label5 = new System.Windows.Forms.Label();
            this.Label3 = new System.Windows.Forms.Label();
            this.lbl_update = new System.Windows.Forms.Label();
            this.btn_close = new System.Windows.Forms.Button();
            this.lbl_GuiVersion = new System.Windows.Forms.Label();
            this.Label4 = new System.Windows.Forms.Label();
            this.Version = new System.Windows.Forms.Label();
            this.Label2 = new System.Windows.Forms.Label();
            this.btn_update = new System.Windows.Forms.Button();
            this.Label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // cliVersion
            // 
            this.cliVersion.AutoSize = true;
            this.cliVersion.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cliVersion.Location = new System.Drawing.Point(371, 58);
            this.cliVersion.Name = "cliVersion";
            this.cliVersion.Size = new System.Drawing.Size(64, 13);
            this.cliVersion.TabIndex = 67;
            this.cliVersion.Text = "{Version}";
            this.cliVersion.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // lbl_cliVersion
            // 
            this.lbl_cliVersion.AutoSize = true;
            this.lbl_cliVersion.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_cliVersion.Location = new System.Drawing.Point(371, 78);
            this.lbl_cliVersion.Name = "lbl_cliVersion";
            this.lbl_cliVersion.Size = new System.Drawing.Size(113, 13);
            this.lbl_cliVersion.TabIndex = 66;
            this.lbl_cliVersion.Text = "Click \"Check Now\"";
            this.lbl_cliVersion.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // Label6
            // 
            this.Label6.AutoSize = true;
            this.Label6.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label6.Location = new System.Drawing.Point(258, 78);
            this.Label6.Name = "Label6";
            this.Label6.Size = new System.Drawing.Size(93, 13);
            this.Label6.TabIndex = 65;
            this.Label6.Text = "Latest Version:";
            this.Label6.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // Label7
            // 
            this.Label7.AutoSize = true;
            this.Label7.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label7.Location = new System.Drawing.Point(258, 58);
            this.Label7.Name = "Label7";
            this.Label7.Size = new System.Drawing.Size(107, 13);
            this.Label7.TabIndex = 64;
            this.Label7.Text = "Current Version: ";
            this.Label7.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // Label5
            // 
            this.Label5.AutoSize = true;
            this.Label5.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label5.Location = new System.Drawing.Point(258, 34);
            this.Label5.Name = "Label5";
            this.Label5.Size = new System.Drawing.Size(163, 13);
            this.Label5.TabIndex = 63;
            this.Label5.Text = "Windows Command Line";
            // 
            // Label3
            // 
            this.Label3.AutoSize = true;
            this.Label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label3.Location = new System.Drawing.Point(14, 34);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(92, 13);
            this.Label3.TabIndex = 62;
            this.Label3.Text = "Windows GUI";
            // 
            // lbl_update
            // 
            this.lbl_update.AutoSize = true;
            this.lbl_update.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_update.ForeColor = System.Drawing.Color.DarkOrange;
            this.lbl_update.Location = new System.Drawing.Point(14, 147);
            this.lbl_update.Name = "lbl_update";
            this.lbl_update.Size = new System.Drawing.Size(0, 13);
            this.lbl_update.TabIndex = 61;
            this.lbl_update.Visible = false;
            // 
            // btn_close
            // 
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(362, 104);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(122, 22);
            this.btn_close.TabIndex = 60;
            this.btn_close.Text = "Close Window";
            this.btn_close.UseVisualStyleBackColor = true;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // lbl_GuiVersion
            // 
            this.lbl_GuiVersion.AutoSize = true;
            this.lbl_GuiVersion.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_GuiVersion.Location = new System.Drawing.Point(127, 78);
            this.lbl_GuiVersion.Name = "lbl_GuiVersion";
            this.lbl_GuiVersion.Size = new System.Drawing.Size(113, 13);
            this.lbl_GuiVersion.TabIndex = 59;
            this.lbl_GuiVersion.Text = "Click \"Check Now\"";
            this.lbl_GuiVersion.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // Label4
            // 
            this.Label4.AutoSize = true;
            this.Label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label4.Location = new System.Drawing.Point(14, 78);
            this.Label4.Name = "Label4";
            this.Label4.Size = new System.Drawing.Size(93, 13);
            this.Label4.TabIndex = 58;
            this.Label4.Text = "Latest Version:";
            this.Label4.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // Version
            // 
            this.Version.AutoSize = true;
            this.Version.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Version.Location = new System.Drawing.Point(127, 58);
            this.Version.Name = "Version";
            this.Version.Size = new System.Drawing.Size(64, 13);
            this.Version.TabIndex = 57;
            this.Version.Text = "{Version}";
            this.Version.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(14, 58);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(107, 13);
            this.Label2.TabIndex = 56;
            this.Label2.Text = "Current Version: ";
            this.Label2.TextAlign = System.Drawing.ContentAlignment.TopCenter;
            // 
            // btn_update
            // 
            this.btn_update.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_update.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_update.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_update.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_update.Location = new System.Drawing.Point(14, 104);
            this.btn_update.Name = "btn_update";
            this.btn_update.Size = new System.Drawing.Size(107, 22);
            this.btn_update.TabIndex = 55;
            this.btn_update.Text = "Check Now";
            this.btn_update.UseVisualStyleBackColor = true;
            this.btn_update.Click += new System.EventHandler(this.btn_update_click);
            // 
            // Label1
            // 
            this.Label1.AutoSize = true;
            this.Label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label1.Location = new System.Drawing.Point(14, 11);
            this.Label1.Name = "Label1";
            this.Label1.Size = new System.Drawing.Size(110, 13);
            this.Label1.TabIndex = 54;
            this.Label1.Text = "Update Checker";
            // 
            // frmUpdate
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(498, 139);
            this.Controls.Add(this.cliVersion);
            this.Controls.Add(this.lbl_cliVersion);
            this.Controls.Add(this.Label6);
            this.Controls.Add(this.Label7);
            this.Controls.Add(this.Label5);
            this.Controls.Add(this.Label3);
            this.Controls.Add(this.lbl_update);
            this.Controls.Add(this.btn_close);
            this.Controls.Add(this.lbl_GuiVersion);
            this.Controls.Add(this.Label4);
            this.Controls.Add(this.Version);
            this.Controls.Add(this.Label2);
            this.Controls.Add(this.btn_update);
            this.Controls.Add(this.Label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(504, 164);
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(504, 164);
            this.Name = "frmUpdate";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Update Check";
            this.Load += new System.EventHandler(this.frmUpdate_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label cliVersion;
        internal System.Windows.Forms.Label lbl_cliVersion;
        internal System.Windows.Forms.Label Label6;
        internal System.Windows.Forms.Label Label7;
        internal System.Windows.Forms.Label Label5;
        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.Label lbl_update;
        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.Label lbl_GuiVersion;
        internal System.Windows.Forms.Label Label4;
        internal System.Windows.Forms.Label Version;
        internal System.Windows.Forms.Label Label2;
        internal System.Windows.Forms.Button btn_update;
        internal System.Windows.Forms.Label Label1;
    }
}