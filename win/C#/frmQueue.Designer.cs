/*  frmQueue.Designer.cs 
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmQueue
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmQueue));
            this.btn_down = new System.Windows.Forms.Button();
            this.btn_up = new System.Windows.Forms.Button();
            this.btn_delete = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.label4 = new System.Windows.Forms.Label();
            this.lbl_chapt = new System.Windows.Forms.Label();
            this.lbl_title = new System.Windows.Forms.Label();
            this.lbl_aEnc = new System.Windows.Forms.Label();
            this.lbl_vEnc = new System.Windows.Forms.Label();
            this.lbl_dest = new System.Windows.Forms.Label();
            this.lbl_source = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.btn_encode = new System.Windows.Forms.ToolStripButton();
            this.btn_stop = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_batch = new System.Windows.Forms.ToolStripButton();
            this.SaveFile = new System.Windows.Forms.SaveFileDialog();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.list_queue = new System.Windows.Forms.ListView();
            this.Title = new System.Windows.Forms.ColumnHeader();
            this.Chapters = new System.Windows.Forms.ColumnHeader();
            this.Source = new System.Windows.Forms.ColumnHeader();
            this.Destination = new System.Windows.Forms.ColumnHeader();
            this.EncoderVideo = new System.Windows.Forms.ColumnHeader();
            this.Audio = new System.Windows.Forms.ColumnHeader();
            this.lbl_progressValue = new System.Windows.Forms.Label();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.label2 = new System.Windows.Forms.Label();
            this.lbl_status = new System.Windows.Forms.Label();
            this.toolStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // btn_down
            // 
            this.btn_down.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_down.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_down.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_down.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_down.Location = new System.Drawing.Point(610, 124);
            this.btn_down.Name = "btn_down";
            this.btn_down.Size = new System.Drawing.Size(75, 22);
            this.btn_down.TabIndex = 33;
            this.btn_down.TabStop = false;
            this.btn_down.Text = "Down";
            this.btn_down.UseVisualStyleBackColor = true;
            this.btn_down.Click += new System.EventHandler(this.btn_down_Click);
            // 
            // btn_up
            // 
            this.btn_up.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_up.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_up.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_up.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_up.Location = new System.Drawing.Point(539, 124);
            this.btn_up.Name = "btn_up";
            this.btn_up.Size = new System.Drawing.Size(64, 22);
            this.btn_up.TabIndex = 32;
            this.btn_up.TabStop = false;
            this.btn_up.Text = "Up";
            this.btn_up.UseVisualStyleBackColor = true;
            this.btn_up.Click += new System.EventHandler(this.btn_up_Click);
            // 
            // btn_delete
            // 
            this.btn_delete.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_delete.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_delete.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_delete.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_delete.Location = new System.Drawing.Point(692, 124);
            this.btn_delete.Name = "btn_delete";
            this.btn_delete.Size = new System.Drawing.Size(75, 22);
            this.btn_delete.TabIndex = 31;
            this.btn_delete.TabStop = false;
            this.btn_delete.Text = "Delete";
            this.btn_delete.UseVisualStyleBackColor = true;
            this.btn_delete.Click += new System.EventHandler(this.btn_delete_Click);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(269, 121);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(47, 26);
            this.label4.TabIndex = 70;
            this.label4.Text = "Video:\r\nAudo:";
            // 
            // lbl_chapt
            // 
            this.lbl_chapt.AutoSize = true;
            this.lbl_chapt.Location = new System.Drawing.Point(202, 133);
            this.lbl_chapt.Name = "lbl_chapt";
            this.lbl_chapt.Size = new System.Drawing.Size(12, 13);
            this.lbl_chapt.TabIndex = 69;
            this.lbl_chapt.Text = "-";
            // 
            // lbl_title
            // 
            this.lbl_title.AccessibleRole = System.Windows.Forms.AccessibleRole.None;
            this.lbl_title.AutoSize = true;
            this.lbl_title.Location = new System.Drawing.Point(202, 120);
            this.lbl_title.Name = "lbl_title";
            this.lbl_title.Size = new System.Drawing.Size(12, 13);
            this.lbl_title.TabIndex = 68;
            this.lbl_title.Text = "-";
            // 
            // lbl_aEnc
            // 
            this.lbl_aEnc.AutoSize = true;
            this.lbl_aEnc.Location = new System.Drawing.Point(330, 134);
            this.lbl_aEnc.Name = "lbl_aEnc";
            this.lbl_aEnc.Size = new System.Drawing.Size(12, 13);
            this.lbl_aEnc.TabIndex = 67;
            this.lbl_aEnc.Text = "-";
            // 
            // lbl_vEnc
            // 
            this.lbl_vEnc.AutoSize = true;
            this.lbl_vEnc.Location = new System.Drawing.Point(330, 121);
            this.lbl_vEnc.Name = "lbl_vEnc";
            this.lbl_vEnc.Size = new System.Drawing.Size(12, 13);
            this.lbl_vEnc.TabIndex = 66;
            this.lbl_vEnc.Text = "-";
            // 
            // lbl_dest
            // 
            this.lbl_dest.AutoSize = true;
            this.lbl_dest.Location = new System.Drawing.Point(202, 108);
            this.lbl_dest.Name = "lbl_dest";
            this.lbl_dest.Size = new System.Drawing.Size(12, 13);
            this.lbl_dest.TabIndex = 65;
            this.lbl_dest.Text = "-";
            this.lbl_dest.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lbl_source
            // 
            this.lbl_source.AutoSize = true;
            this.lbl_source.Location = new System.Drawing.Point(202, 95);
            this.lbl_source.Name = "lbl_source";
            this.lbl_source.Size = new System.Drawing.Size(12, 13);
            this.lbl_source.TabIndex = 64;
            this.lbl_source.Text = "-";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(56, 95);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(99, 52);
            this.label1.TabIndex = 63;
            this.label1.Text = "Source:\r\nDestination:\r\nDVD Title:\r\nDVD Chapters:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(56, 73);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(82, 13);
            this.label3.TabIndex = 62;
            this.label3.Text = "Current Job";
            // 
            // toolStrip1
            // 
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_encode,
            this.btn_stop,
            this.toolStripSeparator1,
            this.btn_batch});
            this.toolStrip1.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.HorizontalStackWithOverflow;
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.toolStrip1.Size = new System.Drawing.Size(780, 49);
            this.toolStrip1.TabIndex = 71;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // btn_encode
            // 
            this.btn_encode.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_encode.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_encode.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_encode.Name = "btn_encode";
            this.btn_encode.Padding = new System.Windows.Forms.Padding(5);
            this.btn_encode.Size = new System.Drawing.Size(88, 46);
            this.btn_encode.Text = "Encode";
            this.btn_encode.Click += new System.EventHandler(this.btn_encode_Click);
            // 
            // btn_stop
            // 
            this.btn_stop.Image = global::Handbrake.Properties.Resources.Pause;
            this.btn_stop.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_stop.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_stop.Name = "btn_stop";
            this.btn_stop.Size = new System.Drawing.Size(72, 46);
            this.btn_stop.Text = "Pause";
            this.btn_stop.Visible = false;
            this.btn_stop.Click += new System.EventHandler(this.btn_stop_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 49);
            // 
            // btn_batch
            // 
            this.btn_batch.Image = global::Handbrake.Properties.Resources.ActivityWindow;
            this.btn_batch.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_batch.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_batch.Name = "btn_batch";
            this.btn_batch.Size = new System.Drawing.Size(100, 46);
            this.btn_batch.Text = "Batch Script";
            this.btn_batch.Click += new System.EventHandler(this.btn_batch_Click);
            // 
            // SaveFile
            // 
            this.SaveFile.Filter = "Batch|.bat";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = global::Handbrake.Properties.Resources.Queue;
            this.pictureBox1.Location = new System.Drawing.Point(12, 61);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(37, 32);
            this.pictureBox1.TabIndex = 61;
            this.pictureBox1.TabStop = false;
            // 
            // list_queue
            // 
            this.list_queue.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.Title,
            this.Chapters,
            this.Source,
            this.Destination,
            this.EncoderVideo,
            this.Audio});
            this.list_queue.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.list_queue.FullRowSelect = true;
            this.list_queue.GridLines = true;
            this.list_queue.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.list_queue.Location = new System.Drawing.Point(12, 157);
            this.list_queue.MultiSelect = false;
            this.list_queue.Name = "list_queue";
            this.list_queue.Size = new System.Drawing.Size(755, 185);
            this.list_queue.TabIndex = 72;
            this.list_queue.UseCompatibleStateImageBehavior = false;
            this.list_queue.View = System.Windows.Forms.View.Details;
            // 
            // Title
            // 
            this.Title.Text = "Title";
            this.Title.Width = 39;
            // 
            // Chapters
            // 
            this.Chapters.Text = "Chapters";
            this.Chapters.Width = 71;
            // 
            // Source
            // 
            this.Source.Text = "Source";
            this.Source.Width = 219;
            // 
            // Destination
            // 
            this.Destination.Text = "Destination";
            this.Destination.Width = 215;
            // 
            // EncoderVideo
            // 
            this.EncoderVideo.Text = "Video Encoder";
            this.EncoderVideo.Width = 110;
            // 
            // Audio
            // 
            this.Audio.Text = "Audio Encoder";
            this.Audio.Width = 94;
            // 
            // lbl_progressValue
            // 
            this.lbl_progressValue.AutoSize = true;
            this.lbl_progressValue.Location = new System.Drawing.Point(737, 353);
            this.lbl_progressValue.Name = "lbl_progressValue";
            this.lbl_progressValue.Size = new System.Drawing.Size(30, 13);
            this.lbl_progressValue.TabIndex = 36;
            this.lbl_progressValue.Text = "0 %";
            // 
            // progressBar
            // 
            this.progressBar.Location = new System.Drawing.Point(76, 348);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(655, 23);
            this.progressBar.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            this.progressBar.TabIndex = 34;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 353);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(62, 13);
            this.label2.TabIndex = 35;
            this.label2.Text = "Progress:";
            // 
            // lbl_status
            // 
            this.lbl_status.AutoSize = true;
            this.lbl_status.BackColor = System.Drawing.Color.Transparent;
            this.lbl_status.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_status.Location = new System.Drawing.Point(319, 353);
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(176, 13);
            this.lbl_status.TabIndex = 42;
            this.lbl_status.Text = "Encode Queue Completed!";
            this.lbl_status.Visible = false;
            // 
            // frmQueue
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(780, 386);
            this.Controls.Add(this.lbl_status);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.list_queue);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.lbl_progressValue);
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.btn_down);
            this.Controls.Add(this.btn_up);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.lbl_dest);
            this.Controls.Add(this.btn_delete);
            this.Controls.Add(this.lbl_vEnc);
            this.Controls.Add(this.lbl_chapt);
            this.Controls.Add(this.lbl_source);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.lbl_aEnc);
            this.Controls.Add(this.lbl_title);
            this.Controls.Add(this.label1);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "frmQueue";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Encode Queue";
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Button btn_down;
        internal System.Windows.Forms.Button btn_up;
        internal System.Windows.Forms.Button btn_delete;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label lbl_chapt;
        private System.Windows.Forms.Label lbl_title;
        private System.Windows.Forms.Label lbl_aEnc;
        private System.Windows.Forms.Label lbl_vEnc;
        private System.Windows.Forms.Label lbl_dest;
        private System.Windows.Forms.Label lbl_source;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton btn_encode;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton btn_stop;
        private System.Windows.Forms.ToolStripButton btn_batch;
        private System.Windows.Forms.SaveFileDialog SaveFile;
        private System.Windows.Forms.ListView list_queue;
        private System.Windows.Forms.ColumnHeader Title;
        private System.Windows.Forms.ColumnHeader Chapters;
        private System.Windows.Forms.ColumnHeader Source;
        private System.Windows.Forms.ColumnHeader Destination;
        private System.Windows.Forms.ColumnHeader EncoderVideo;
        private System.Windows.Forms.ColumnHeader Audio;
        private System.Windows.Forms.Label lbl_progressValue;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label lbl_status;
    }
}