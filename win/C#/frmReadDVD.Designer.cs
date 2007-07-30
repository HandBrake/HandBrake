namespace Handbrake
{
    partial class frmReadDVD
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmReadDVD));
            this.lbl_pressOk = new System.Windows.Forms.Label();
            this.btn_ok = new System.Windows.Forms.Button();
            this.Label3 = new System.Windows.Forms.Label();
            this.Label2 = new System.Windows.Forms.Label();
            this.lbl_status = new System.Windows.Forms.Label();
            this.lbl_progress = new System.Windows.Forms.Label();
            this.btn_skip = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // lbl_pressOk
            // 
            this.lbl_pressOk.AutoSize = true;
            this.lbl_pressOk.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_pressOk.Location = new System.Drawing.Point(77, 61);
            this.lbl_pressOk.Name = "lbl_pressOk";
            this.lbl_pressOk.Size = new System.Drawing.Size(178, 13);
            this.lbl_pressOk.TabIndex = 29;
            this.lbl_pressOk.Text = "Press OK to start the process.";
            // 
            // btn_ok
            // 
            this.btn_ok.BackColor = System.Drawing.SystemColors.Control;
            this.btn_ok.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_ok.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_ok.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_ok.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_ok.Location = new System.Drawing.Point(351, 56);
            this.btn_ok.Name = "btn_ok";
            this.btn_ok.Size = new System.Drawing.Size(71, 22);
            this.btn_ok.TabIndex = 28;
            this.btn_ok.TabStop = false;
            this.btn_ok.Text = "OK";
            this.btn_ok.UseVisualStyleBackColor = false;
            this.btn_ok.Click += new System.EventHandler(this.btn_ok_Click);
            // 
            // Label3
            // 
            this.Label3.AutoSize = true;
            this.Label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label3.Location = new System.Drawing.Point(77, 9);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(222, 26);
            this.Label3.TabIndex = 27;
            this.Label3.Text = "The CLI is about to read the DVD...\r\nThis process can take up to 1 minute.";
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(9, 8);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(52, 13);
            this.Label2.TabIndex = 26;
            this.Label2.Text = "Status:";
            // 
            // lbl_status
            // 
            this.lbl_status.AutoSize = true;
            this.lbl_status.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_status.Location = new System.Drawing.Point(77, 41);
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(178, 13);
            this.lbl_status.TabIndex = 31;
            this.lbl_status.Text = "Processing.... Please Wait!";
            this.lbl_status.Visible = false;
            // 
            // lbl_progress
            // 
            this.lbl_progress.AutoSize = true;
            this.lbl_progress.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_progress.Location = new System.Drawing.Point(285, 41);
            this.lbl_progress.Name = "lbl_progress";
            this.lbl_progress.Size = new System.Drawing.Size(45, 13);
            this.lbl_progress.TabIndex = 32;
            this.lbl_progress.Text = "{ % }";
            this.lbl_progress.Visible = false;
            // 
            // btn_skip
            // 
            this.btn_skip.BackColor = System.Drawing.SystemColors.Control;
            this.btn_skip.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_skip.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_skip.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_skip.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_skip.Location = new System.Drawing.Point(351, 4);
            this.btn_skip.Name = "btn_skip";
            this.btn_skip.Size = new System.Drawing.Size(71, 22);
            this.btn_skip.TabIndex = 33;
            this.btn_skip.TabStop = false;
            this.btn_skip.Text = "Cancel";
            this.btn_skip.UseVisualStyleBackColor = true;
            this.btn_skip.Visible = false;
            this.btn_skip.Click += new System.EventHandler(this.btn_skip_Click);
            // 
            // frmReadDVD
            // 
            this.AcceptButton = this.btn_ok;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(434, 88);
            this.ControlBox = false;
            this.Controls.Add(this.btn_skip);
            this.Controls.Add(this.lbl_progress);
            this.Controls.Add(this.lbl_status);
            this.Controls.Add(this.lbl_pressOk);
            this.Controls.Add(this.btn_ok);
            this.Controls.Add(this.Label3);
            this.Controls.Add(this.Label2);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(538, 113);
            this.Name = "frmReadDVD";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Reading DVD...";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label lbl_pressOk;
        internal System.Windows.Forms.Button btn_ok;
        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.Label Label2;
        internal System.Windows.Forms.Label lbl_status;
        private System.Windows.Forms.Label lbl_progress;
        internal System.Windows.Forms.Button btn_skip;
    }
}