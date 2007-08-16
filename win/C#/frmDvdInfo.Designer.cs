namespace Handbrake
{
    partial class frmDvdInfo
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmDvdInfo));
            this.Label2 = new System.Windows.Forms.Label();
            this.rtf_dvdInfo = new System.Windows.Forms.RichTextBox();
            this.btn_close = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.BackColor = System.Drawing.SystemColors.ControlLight;
            this.Label2.Location = new System.Drawing.Point(13, 14);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(518, 26);
            this.Label2.TabIndex = 30;
            this.Label2.Text = "Handbrake\'s DVD information output in an unparsed form.\r\nNote if you have not sca" +
                "nned the DVD this feature will display the information for the last DVD that was" +
                " read.";
            // 
            // rtf_dvdInfo
            // 
            this.rtf_dvdInfo.DetectUrls = false;
            this.rtf_dvdInfo.Location = new System.Drawing.Point(16, 51);
            this.rtf_dvdInfo.Name = "rtf_dvdInfo";
            this.rtf_dvdInfo.ReadOnly = true;
            this.rtf_dvdInfo.Size = new System.Drawing.Size(515, 395);
            this.rtf_dvdInfo.TabIndex = 29;
            this.rtf_dvdInfo.Text = "";
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(421, 452);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(110, 22);
            this.btn_close.TabIndex = 28;
            this.btn_close.TabStop = false;
            this.btn_close.Text = "Close Window";
            this.btn_close.UseVisualStyleBackColor = false;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // frmDvdInfo
            // 
            this.AcceptButton = this.btn_close;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(547, 495);
            this.Controls.Add(this.Label2);
            this.Controls.Add(this.rtf_dvdInfo);
            this.Controls.Add(this.btn_close);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(553, 520);
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(553, 520);
            this.Name = "frmDvdInfo";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Read DVD";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label Label2;
        internal System.Windows.Forms.RichTextBox rtf_dvdInfo;
        internal System.Windows.Forms.Button btn_close;
    }
}