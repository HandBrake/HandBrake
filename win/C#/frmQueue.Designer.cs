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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmQueue));
            this.label3 = new System.Windows.Forms.Label();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.listview_queue = new System.Windows.Forms.ListView();
            this.c_job = new System.Windows.Forms.ColumnHeader();
            this.btn_delete = new System.Windows.Forms.Button();
            this.btn_up = new System.Windows.Forms.Button();
            this.btn_down = new System.Windows.Forms.Button();
            this.lbl_status = new System.Windows.Forms.Label();
            this.btn_cancel = new System.Windows.Forms.Button();
            this.lbl_progressValue = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.btn_q_encoder = new System.Windows.Forms.Button();
            this.btn_Close = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.lbl_source = new System.Windows.Forms.Label();
            this.lbl_dest = new System.Windows.Forms.Label();
            this.lbl_vEnc = new System.Windows.Forms.Label();
            this.lbl_aEnc = new System.Windows.Forms.Label();
            this.lbl_chapt = new System.Windows.Forms.Label();
            this.lbl_title = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.txt_editQuery = new System.Windows.Forms.TextBox();
            this.btn_update = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(50, 24);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(82, 13);
            this.label3.TabIndex = 41;
            this.label3.Text = "Current Job";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = global::Handbrake.Properties.Resources.Queue;
            this.pictureBox1.Location = new System.Drawing.Point(12, 12);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(32, 32);
            this.pictureBox1.TabIndex = 40;
            this.pictureBox1.TabStop = false;
            // 
            // listview_queue
            // 
            this.listview_queue.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.listview_queue.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.c_job});
            this.listview_queue.Location = new System.Drawing.Point(12, 116);
            this.listview_queue.MultiSelect = false;
            this.listview_queue.Name = "listview_queue";
            this.listview_queue.ShowItemToolTips = true;
            this.listview_queue.Size = new System.Drawing.Size(707, 146);
            this.listview_queue.TabIndex = 42;
            this.listview_queue.UseCompatibleStateImageBehavior = false;
            this.listview_queue.View = System.Windows.Forms.View.Details;
            this.listview_queue.SelectedIndexChanged += new System.EventHandler(this.listview_queue_SelectedIndexChanged);
            // 
            // c_job
            // 
            this.c_job.Text = "Job";
            this.c_job.Width = 721;
            // 
            // btn_delete
            // 
            this.btn_delete.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_delete.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_delete.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_delete.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_delete.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_delete.Location = new System.Drawing.Point(152, 268);
            this.btn_delete.Name = "btn_delete";
            this.btn_delete.Size = new System.Drawing.Size(64, 22);
            this.btn_delete.TabIndex = 43;
            this.btn_delete.TabStop = false;
            this.btn_delete.Text = "Delete";
            this.btn_delete.UseVisualStyleBackColor = false;
            this.btn_delete.Click += new System.EventHandler(this.btn_delete_Click);
            // 
            // btn_up
            // 
            this.btn_up.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_up.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_up.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_up.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_up.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_up.Location = new System.Drawing.Point(12, 268);
            this.btn_up.Name = "btn_up";
            this.btn_up.Size = new System.Drawing.Size(64, 22);
            this.btn_up.TabIndex = 44;
            this.btn_up.TabStop = false;
            this.btn_up.Text = "Up";
            this.btn_up.UseVisualStyleBackColor = false;
            this.btn_up.Click += new System.EventHandler(this.btn_up_Click);
            // 
            // btn_down
            // 
            this.btn_down.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_down.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_down.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_down.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_down.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_down.Location = new System.Drawing.Point(82, 268);
            this.btn_down.Name = "btn_down";
            this.btn_down.Size = new System.Drawing.Size(64, 22);
            this.btn_down.TabIndex = 45;
            this.btn_down.TabStop = false;
            this.btn_down.Text = "Down";
            this.btn_down.UseVisualStyleBackColor = false;
            this.btn_down.Click += new System.EventHandler(this.btn_down_Click);
            // 
            // lbl_status
            // 
            this.lbl_status.AutoSize = true;
            this.lbl_status.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_status.Location = new System.Drawing.Point(273, 304);
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(176, 13);
            this.lbl_status.TabIndex = 52;
            this.lbl_status.Text = "Encode Queue Completed!";
            this.lbl_status.Visible = false;
            // 
            // btn_cancel
            // 
            this.btn_cancel.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_cancel.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_cancel.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_cancel.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_cancel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_cancel.Location = new System.Drawing.Point(142, 336);
            this.btn_cancel.Name = "btn_cancel";
            this.btn_cancel.Size = new System.Drawing.Size(91, 22);
            this.btn_cancel.TabIndex = 51;
            this.btn_cancel.TabStop = false;
            this.btn_cancel.Text = "Stop Queue";
            this.btn_cancel.UseVisualStyleBackColor = false;
            this.btn_cancel.Visible = false;
            this.btn_cancel.Click += new System.EventHandler(this.btn_cancel_Click);
            // 
            // lbl_progressValue
            // 
            this.lbl_progressValue.AutoSize = true;
            this.lbl_progressValue.Location = new System.Drawing.Point(689, 304);
            this.lbl_progressValue.Name = "lbl_progressValue";
            this.lbl_progressValue.Size = new System.Drawing.Size(30, 13);
            this.lbl_progressValue.TabIndex = 50;
            this.lbl_progressValue.Text = "0 %";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 304);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(62, 13);
            this.label2.TabIndex = 49;
            this.label2.Text = "Progress:";
            // 
            // progressBar
            // 
            this.progressBar.BackColor = System.Drawing.SystemColors.ControlLight;
            this.progressBar.Location = new System.Drawing.Point(75, 299);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(608, 23);
            this.progressBar.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            this.progressBar.TabIndex = 48;
            // 
            // btn_q_encoder
            // 
            this.btn_q_encoder.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_q_encoder.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_q_encoder.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_q_encoder.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_q_encoder.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_q_encoder.Location = new System.Drawing.Point(11, 336);
            this.btn_q_encoder.Name = "btn_q_encoder";
            this.btn_q_encoder.Size = new System.Drawing.Size(124, 22);
            this.btn_q_encoder.TabIndex = 47;
            this.btn_q_encoder.TabStop = false;
            this.btn_q_encoder.Text = "Encode Video(s)";
            this.btn_q_encoder.UseVisualStyleBackColor = false;
            this.btn_q_encoder.Click += new System.EventHandler(this.btn_q_encoder_Click);
            // 
            // btn_Close
            // 
            this.btn_Close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_Close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_Close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_Close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_Close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Close.Location = new System.Drawing.Point(605, 336);
            this.btn_Close.Name = "btn_Close";
            this.btn_Close.Size = new System.Drawing.Size(108, 22);
            this.btn_Close.TabIndex = 46;
            this.btn_Close.TabStop = false;
            this.btn_Close.Text = "Close Window";
            this.btn_Close.UseVisualStyleBackColor = false;
            this.btn_Close.Click += new System.EventHandler(this.btn_Close_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(50, 46);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(99, 52);
            this.label1.TabIndex = 53;
            this.label1.Text = "Source:\r\nDestination:\r\nDVD Title:\r\nDVD Chapters:";
            // 
            // lbl_source
            // 
            this.lbl_source.AutoSize = true;
            this.lbl_source.Location = new System.Drawing.Point(175, 46);
            this.lbl_source.Name = "lbl_source";
            this.lbl_source.Size = new System.Drawing.Size(12, 13);
            this.lbl_source.TabIndex = 54;
            this.lbl_source.Text = "-";
            // 
            // lbl_dest
            // 
            this.lbl_dest.AutoSize = true;
            this.lbl_dest.Location = new System.Drawing.Point(175, 59);
            this.lbl_dest.Name = "lbl_dest";
            this.lbl_dest.Size = new System.Drawing.Size(12, 13);
            this.lbl_dest.TabIndex = 55;
            this.lbl_dest.Text = "-";
            this.lbl_dest.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lbl_vEnc
            // 
            this.lbl_vEnc.AutoSize = true;
            this.lbl_vEnc.Location = new System.Drawing.Point(285, 72);
            this.lbl_vEnc.Name = "lbl_vEnc";
            this.lbl_vEnc.Size = new System.Drawing.Size(12, 13);
            this.lbl_vEnc.TabIndex = 56;
            this.lbl_vEnc.Text = "-";
            // 
            // lbl_aEnc
            // 
            this.lbl_aEnc.AutoSize = true;
            this.lbl_aEnc.Location = new System.Drawing.Point(285, 85);
            this.lbl_aEnc.Name = "lbl_aEnc";
            this.lbl_aEnc.Size = new System.Drawing.Size(12, 13);
            this.lbl_aEnc.TabIndex = 57;
            this.lbl_aEnc.Text = "-";
            // 
            // lbl_chapt
            // 
            this.lbl_chapt.AutoSize = true;
            this.lbl_chapt.Location = new System.Drawing.Point(175, 84);
            this.lbl_chapt.Name = "lbl_chapt";
            this.lbl_chapt.Size = new System.Drawing.Size(12, 13);
            this.lbl_chapt.TabIndex = 59;
            this.lbl_chapt.Text = "-";
            // 
            // lbl_title
            // 
            this.lbl_title.AccessibleRole = System.Windows.Forms.AccessibleRole.None;
            this.lbl_title.AutoSize = true;
            this.lbl_title.Location = new System.Drawing.Point(175, 71);
            this.lbl_title.Name = "lbl_title";
            this.lbl_title.Size = new System.Drawing.Size(12, 13);
            this.lbl_title.TabIndex = 58;
            this.lbl_title.Text = "-";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(232, 72);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(47, 26);
            this.label4.TabIndex = 60;
            this.label4.Text = "Video:\r\nAudo:";
            // 
            // txt_editQuery
            // 
            this.txt_editQuery.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.txt_editQuery.Location = new System.Drawing.Point(222, 269);
            this.txt_editQuery.Name = "txt_editQuery";
            this.txt_editQuery.Size = new System.Drawing.Size(427, 21);
            this.txt_editQuery.TabIndex = 61;
            // 
            // btn_update
            // 
            this.btn_update.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_update.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_update.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_update.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_update.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_update.Location = new System.Drawing.Point(655, 268);
            this.btn_update.Name = "btn_update";
            this.btn_update.Size = new System.Drawing.Size(64, 22);
            this.btn_update.TabIndex = 62;
            this.btn_update.TabStop = false;
            this.btn_update.Text = "Update";
            this.btn_update.UseVisualStyleBackColor = false;
            this.btn_update.Click += new System.EventHandler(this.btn_update_Click);
            // 
            // frmQueue
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(729, 371);
            this.Controls.Add(this.btn_update);
            this.Controls.Add(this.txt_editQuery);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.lbl_chapt);
            this.Controls.Add(this.lbl_title);
            this.Controls.Add(this.lbl_aEnc);
            this.Controls.Add(this.lbl_vEnc);
            this.Controls.Add(this.lbl_dest);
            this.Controls.Add(this.lbl_source);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lbl_status);
            this.Controls.Add(this.btn_cancel);
            this.Controls.Add(this.lbl_progressValue);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.btn_q_encoder);
            this.Controls.Add(this.btn_Close);
            this.Controls.Add(this.btn_delete);
            this.Controls.Add(this.btn_up);
            this.Controls.Add(this.btn_down);
            this.Controls.Add(this.listview_queue);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.pictureBox1);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmQueue";
            this.Text = "Queue";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.ColumnHeader c_job;
        public System.Windows.Forms.ListView listview_queue;
        internal System.Windows.Forms.Button btn_delete;
        internal System.Windows.Forms.Button btn_up;
        internal System.Windows.Forms.Button btn_down;
        private System.Windows.Forms.Label lbl_status;
        internal System.Windows.Forms.Button btn_cancel;
        private System.Windows.Forms.Label lbl_progressValue;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ProgressBar progressBar;
        internal System.Windows.Forms.Button btn_q_encoder;
        internal System.Windows.Forms.Button btn_Close;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lbl_source;
        private System.Windows.Forms.Label lbl_dest;
        private System.Windows.Forms.Label lbl_vEnc;
        private System.Windows.Forms.Label lbl_aEnc;
        private System.Windows.Forms.Label lbl_chapt;
        private System.Windows.Forms.Label lbl_title;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox txt_editQuery;
        internal System.Windows.Forms.Button btn_update;
    }
}