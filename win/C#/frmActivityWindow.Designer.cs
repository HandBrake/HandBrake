/*  frmDvdInfo.Designer.cs 
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.m0k.org/>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmActivityWindow
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmActivityWindow));
            this.rtf_actLog = new System.Windows.Forms.RichTextBox();
            this.btn_close = new System.Windows.Forms.Button();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.btn_copy = new System.Windows.Forms.Button();
            this.btn_refresh = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // rtf_actLog
            // 
            this.rtf_actLog.DetectUrls = false;
            this.rtf_actLog.Location = new System.Drawing.Point(12, 12);
            this.rtf_actLog.Name = "rtf_actLog";
            this.rtf_actLog.ReadOnly = true;
            this.rtf_actLog.Size = new System.Drawing.Size(515, 433);
            this.rtf_actLog.TabIndex = 29;
            this.rtf_actLog.Text = "";
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(399, 451);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(128, 22);
            this.btn_close.TabIndex = 28;
            this.btn_close.TabStop = false;
            this.btn_close.Text = "Close Window";
            this.btn_close.UseVisualStyleBackColor = false;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            // 
            // btn_copy
            // 
            this.btn_copy.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_copy.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_copy.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_copy.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_copy.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_copy.Location = new System.Drawing.Point(12, 451);
            this.btn_copy.Name = "btn_copy";
            this.btn_copy.Size = new System.Drawing.Size(134, 23);
            this.btn_copy.TabIndex = 93;
            this.btn_copy.Text = "Copy to Clipboard";
            this.btn_copy.UseVisualStyleBackColor = false;
            this.btn_copy.Click += new System.EventHandler(this.btn_copy_Click);
            // 
            // btn_refresh
            // 
            this.btn_refresh.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_refresh.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_refresh.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_refresh.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_refresh.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_refresh.Location = new System.Drawing.Point(152, 451);
            this.btn_refresh.Name = "btn_refresh";
            this.btn_refresh.Size = new System.Drawing.Size(69, 23);
            this.btn_refresh.TabIndex = 94;
            this.btn_refresh.Text = "Refresh";
            this.btn_refresh.UseVisualStyleBackColor = false;
            this.btn_refresh.Click += new System.EventHandler(this.btn_refresh_Click);
            // 
            // frmActivityWindow
            // 
            this.AcceptButton = this.btn_close;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(539, 486);
            this.Controls.Add(this.btn_refresh);
            this.Controls.Add(this.btn_copy);
            this.Controls.Add(this.rtf_actLog);
            this.Controls.Add(this.btn_close);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmActivityWindow";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Activity Window";
            this.Load += new System.EventHandler(this.frmActivityWindow_Load);
            this.ResumeLayout(false);

        }

        #endregion

        internal System.Windows.Forms.RichTextBox rtf_actLog;
        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.ToolTip ToolTip;
        internal System.Windows.Forms.Button btn_copy;
        internal System.Windows.Forms.Button btn_refresh;
    }
}