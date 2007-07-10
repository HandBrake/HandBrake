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
            this.SuspendLayout();
            // 
            // lbl_pressOk
            // 
            this.lbl_pressOk.AutoSize = true;
            this.lbl_pressOk.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_pressOk.Location = new System.Drawing.Point(66, 60);
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
            this.btn_ok.Location = new System.Drawing.Point(405, 51);
            this.btn_ok.Name = "btn_ok";
            this.btn_ok.Size = new System.Drawing.Size(56, 22);
            this.btn_ok.TabIndex = 28;
            this.btn_ok.TabStop = false;
            this.btn_ok.Text = "Ok";
            this.btn_ok.UseVisualStyleBackColor = false;
            this.btn_ok.Click += new System.EventHandler(this.btn_ok_Click);
            // 
            // Label3
            // 
            this.Label3.AutoSize = true;
            this.Label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label3.Location = new System.Drawing.Point(66, 9);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(222, 26);
            this.Label3.TabIndex = 27;
            this.Label3.Text = "The CLI is about to read the DVD...\r\nThis process can take up to 1 minute.";
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(8, 8);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(52, 13);
            this.Label2.TabIndex = 26;
            this.Label2.Text = "Status:";
            // 
            // lbl_status
            // 
            this.lbl_status.AutoSize = true;
            this.lbl_status.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_status.Location = new System.Drawing.Point(66, 41);
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(178, 13);
            this.lbl_status.TabIndex = 30;
            this.lbl_status.Text = "Processing.... Please Wait!";
            this.lbl_status.Visible = false;
            // 
            // frmReadDVD
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(473, 86);
            this.Controls.Add(this.lbl_status);
            this.Controls.Add(this.lbl_pressOk);
            this.Controls.Add(this.btn_ok);
            this.Controls.Add(this.Label3);
            this.Controls.Add(this.Label2);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmReadDVD";
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
    }
}