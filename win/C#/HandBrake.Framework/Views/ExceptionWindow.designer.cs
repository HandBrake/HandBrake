namespace HandBrake.Framework.Views
{
    partial class ExceptionWindow
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExceptionWindow));
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.PictureBox1 = new System.Windows.Forms.PictureBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.lbl_shortError = new System.Windows.Forms.Label();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel5 = new System.Windows.Forms.Panel();
            this.panel4 = new System.Windows.Forms.Panel();
            this.panel6 = new System.Windows.Forms.Panel();
            this.btn_close = new System.Windows.Forms.Button();
            this.btn_copy = new System.Windows.Forms.Button();
            this.rtf_exceptionFull = new System.Windows.Forms.RichTextBox();
            this.mnu_copy_log = new System.Windows.Forms.ToolStripMenuItem();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).BeginInit();
            this.panel4.SuspendLayout();
            this.panel6.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.White;
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Controls.Add(this.PictureBox1);
            this.panel1.Controls.Add(this.label3);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Controls.Add(this.lbl_shortError);
            resources.ApplyResources(this.panel1, "panel1");
            this.panel1.Name = "panel1";
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.SystemColors.Control;
            resources.ApplyResources(this.panel2, "panel2");
            this.panel2.MaximumSize = new System.Drawing.Size(0, 10);
            this.panel2.MinimumSize = new System.Drawing.Size(0, 10);
            this.panel2.Name = "panel2";
            // 
            // PictureBox1
            // 
            this.PictureBox1.Image = global::HandBrake.Framework.Properties.Resources.ErrorX;
            resources.ApplyResources(this.PictureBox1, "PictureBox1");
            this.PictureBox1.Name = "PictureBox1";
            this.PictureBox1.TabStop = false;
            // 
            // label3
            // 
            resources.ApplyResources(this.label3, "label3");
            this.label3.Name = "label3";
            // 
            // label1
            // 
            resources.ApplyResources(this.label1, "label1");
            this.label1.Name = "label1";
            // 
            // lbl_shortError
            // 
            resources.ApplyResources(this.lbl_shortError, "lbl_shortError");
            this.lbl_shortError.Name = "lbl_shortError";
            // 
            // panel3
            // 
            resources.ApplyResources(this.panel3, "panel3");
            this.panel3.Name = "panel3";
            // 
            // panel5
            // 
            resources.ApplyResources(this.panel5, "panel5");
            this.panel5.Name = "panel5";
            // 
            // panel4
            // 
            this.panel4.Controls.Add(this.panel6);
            resources.ApplyResources(this.panel4, "panel4");
            this.panel4.Name = "panel4";
            // 
            // panel6
            // 
            this.panel6.Controls.Add(this.btn_close);
            this.panel6.Controls.Add(this.btn_copy);
            resources.ApplyResources(this.panel6, "panel6");
            this.panel6.Name = "panel6";
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.Color.Transparent;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            resources.ApplyResources(this.btn_close, "btn_close");
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Name = "btn_close";
            this.btn_close.UseVisualStyleBackColor = false;
            this.btn_close.Click += new System.EventHandler(this.BtnCloseClick);
            // 
            // btn_copy
            // 
            this.btn_copy.BackColor = System.Drawing.Color.Transparent;
            this.btn_copy.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            resources.ApplyResources(this.btn_copy, "btn_copy");
            this.btn_copy.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_copy.Image = global::HandBrake.Framework.Properties.Resources.copy;
            this.btn_copy.Name = "btn_copy";
            this.btn_copy.UseVisualStyleBackColor = false;
            this.btn_copy.Click += new System.EventHandler(this.BtnCopyClick);
            // 
            // rtf_exceptionFull
            // 
            resources.ApplyResources(this.rtf_exceptionFull, "rtf_exceptionFull");
            this.rtf_exceptionFull.Name = "rtf_exceptionFull";
            // 
            // mnu_copy_log
            // 
            this.mnu_copy_log.Image = global::HandBrake.Framework.Properties.Resources.copy;
            this.mnu_copy_log.Name = "mnu_copy_log";
            resources.ApplyResources(this.mnu_copy_log, "mnu_copy_log");
            this.mnu_copy_log.Click += new System.EventHandler(this.MnuCopyLogClick);
            // 
            // ExceptionWindow
            // 
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.rtf_exceptionFull);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.panel4);
            this.Controls.Add(this.panel1);
            this.Name = "ExceptionWindow";
            this.TopMost = true;
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).EndInit();
            this.panel4.ResumeLayout(false);
            this.panel6.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel2;
        internal System.Windows.Forms.PictureBox PictureBox1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lbl_shortError;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Panel panel5;
        private System.Windows.Forms.Panel panel4;
        private System.Windows.Forms.RichTextBox rtf_exceptionFull;
        internal System.Windows.Forms.Button btn_close;
        private System.Windows.Forms.Panel panel6;
        internal System.Windows.Forms.Button btn_copy;
        private System.Windows.Forms.ContextMenuStrip rightClickMenu;
        private System.Windows.Forms.ToolStripMenuItem mnu_copy_log;
    }
}