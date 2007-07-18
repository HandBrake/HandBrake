namespace Handbrake
{
    partial class frmSelectDVD
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmSelectDVD));
            this.btn_close = new System.Windows.Forms.Button();
            this.Label1 = new System.Windows.Forms.Label();
            this.DVD_Open = new System.Windows.Forms.FolderBrowserDialog();
            this.ISO_Open = new System.Windows.Forms.OpenFileDialog();
            this.RadioDVD = new System.Windows.Forms.RadioButton();
            this.RadioISO = new System.Windows.Forms.RadioButton();
            this.btn_Browse = new System.Windows.Forms.Button();
            this.text_source = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // btn_close
            // 
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(317, 65);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(78, 22);
            this.btn_close.TabIndex = 54;
            this.btn_close.Text = "Close";
            this.btn_close.UseVisualStyleBackColor = true;
            // 
            // Label1
            // 
            this.Label1.AutoSize = true;
            this.Label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label1.Location = new System.Drawing.Point(9, 12);
            this.Label1.Name = "Label1";
            this.Label1.Size = new System.Drawing.Size(126, 13);
            this.Label1.TabIndex = 53;
            this.Label1.Text = "Select DVD Source";
            // 
            // ISO_Open
            // 
            this.ISO_Open.DefaultExt = "iso";
            this.ISO_Open.Filter = "All Supported Files|*.iso;*.mpg;*.mpeg;*.vob";
            // 
            // RadioDVD
            // 
            this.RadioDVD.AutoSize = true;
            this.RadioDVD.Checked = true;
            this.RadioDVD.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.RadioDVD.Location = new System.Drawing.Point(261, 29);
            this.RadioDVD.Name = "RadioDVD";
            this.RadioDVD.Size = new System.Drawing.Size(51, 17);
            this.RadioDVD.TabIndex = 52;
            this.RadioDVD.TabStop = true;
            this.RadioDVD.Text = "DVD";
            this.RadioDVD.UseVisualStyleBackColor = true;
            // 
            // RadioISO
            // 
            this.RadioISO.AutoSize = true;
            this.RadioISO.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.RadioISO.Location = new System.Drawing.Point(261, 45);
            this.RadioISO.Name = "RadioISO";
            this.RadioISO.Size = new System.Drawing.Size(44, 17);
            this.RadioISO.TabIndex = 51;
            this.RadioISO.Text = "File";
            this.RadioISO.UseVisualStyleBackColor = true;
            // 
            // btn_Browse
            // 
            this.btn_Browse.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_Browse.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_Browse.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_Browse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Browse.Location = new System.Drawing.Point(317, 34);
            this.btn_Browse.Name = "btn_Browse";
            this.btn_Browse.Size = new System.Drawing.Size(78, 22);
            this.btn_Browse.TabIndex = 49;
            this.btn_Browse.Text = "Browse";
            this.btn_Browse.UseVisualStyleBackColor = true;
            // 
            // text_source
            // 
            this.text_source.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_source.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.text_source.Location = new System.Drawing.Point(12, 35);
            this.text_source.Name = "text_source";
            this.text_source.Size = new System.Drawing.Size(242, 21);
            this.text_source.TabIndex = 48;
            // 
            // frmSelectDVD
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(402, 94);
            this.Controls.Add(this.btn_close);
            this.Controls.Add(this.Label1);
            this.Controls.Add(this.RadioDVD);
            this.Controls.Add(this.RadioISO);
            this.Controls.Add(this.btn_Browse);
            this.Controls.Add(this.text_source);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(410, 121);
            this.MinimumSize = new System.Drawing.Size(410, 121);
            this.Name = "frmSelectDVD";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Read DVD";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.Label Label1;
        internal System.Windows.Forms.FolderBrowserDialog DVD_Open;
        internal System.Windows.Forms.OpenFileDialog ISO_Open;
        internal System.Windows.Forms.RadioButton RadioDVD;
        internal System.Windows.Forms.RadioButton RadioISO;
        internal System.Windows.Forms.Button btn_Browse;
        internal System.Windows.Forms.TextBox text_source;
    }
}