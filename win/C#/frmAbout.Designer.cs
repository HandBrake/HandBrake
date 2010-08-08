/*  frmAbout.Designer.cs 
 	
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

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
            this.Label3 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.PictureBox1 = new System.Windows.Forms.PictureBox();
            this.btn_close = new System.Windows.Forms.Button();
            this.richTextBox1 = new System.Windows.Forms.RichTextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.lbl_GUIBuild = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // Label3
            // 
            this.Label3.AutoSize = true;
            this.Label3.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label3.Location = new System.Drawing.Point(124, 12);
            this.Label3.Margin = new System.Windows.Forms.Padding(3, 3, 3, 1);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(99, 19);
            this.Label3.TabIndex = 25;
            this.Label3.Text = "HandBrake";
            this.Label3.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(125, 36);
            this.label1.Margin = new System.Windows.Forms.Padding(3);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(224, 13);
            this.label1.TabIndex = 33;
            this.label1.Text = "Copyright  2003-2010 HandBrake Developers";
            this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // PictureBox1
            // 
            this.PictureBox1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.PictureBox1.Image = global::Handbrake.Properties.Resources.logo128;
            this.PictureBox1.InitialImage = null;
            this.PictureBox1.Location = new System.Drawing.Point(9, 9);
            this.PictureBox1.Margin = new System.Windows.Forms.Padding(0);
            this.PictureBox1.Name = "PictureBox1";
            this.PictureBox1.Size = new System.Drawing.Size(96, 96);
            this.PictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.PictureBox1.TabIndex = 33;
            this.PictureBox1.TabStop = false;
            // 
            // btn_close
            // 
            this.btn_close.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btn_close.Location = new System.Drawing.Point(468, 270);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(75, 23);
            this.btn_close.TabIndex = 35;
            this.btn_close.Text = "OK";
            this.btn_close.UseVisualStyleBackColor = true;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // richTextBox1
            // 
            this.richTextBox1.Location = new System.Drawing.Point(128, 102);
            this.richTextBox1.Name = "richTextBox1";
            this.richTextBox1.Size = new System.Drawing.Size(415, 162);
            this.richTextBox1.TabIndex = 36;
            this.richTextBox1.Text = resources.GetString("richTextBox1.Text");
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(125, 83);
            this.label2.Margin = new System.Windows.Forms.Padding(3);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(46, 13);
            this.label2.TabIndex = 37;
            this.label2.Text = "License:";
            this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lbl_GUIBuild
            // 
            this.lbl_GUIBuild.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_GUIBuild.Location = new System.Drawing.Point(229, 17);
            this.lbl_GUIBuild.Margin = new System.Windows.Forms.Padding(3, 1, 3, 3);
            this.lbl_GUIBuild.Name = "lbl_GUIBuild";
            this.lbl_GUIBuild.Size = new System.Drawing.Size(224, 13);
            this.lbl_GUIBuild.TabIndex = 38;
            this.lbl_GUIBuild.Text = "{GUI Version}";
            this.lbl_GUIBuild.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // frmAbout
            // 
            this.AcceptButton = this.btn_close;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.CancelButton = this.btn_close;
            this.ClientSize = new System.Drawing.Size(555, 302);
            this.Controls.Add(this.lbl_GUIBuild);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.richTextBox1);
            this.Controls.Add(this.btn_close);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.PictureBox1);
            this.Controls.Add(this.Label3);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmAbout";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "About HandBrake";
            ((System.ComponentModel.ISupportInitialize)(this.PictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.Label label1;
        internal System.Windows.Forms.PictureBox PictureBox1;
        private System.Windows.Forms.Button btn_close;
        private System.Windows.Forms.RichTextBox richTextBox1;
        internal System.Windows.Forms.Label label2;
        internal System.Windows.Forms.Label lbl_GUIBuild;
    }
}