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
            this.btn_q_encoder = new System.Windows.Forms.Button();
            this.list_queue = new System.Windows.Forms.ListBox();
            this.btn_Close = new System.Windows.Forms.Button();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.label2 = new System.Windows.Forms.Label();
            this.lbl_progressValue = new System.Windows.Forms.Label();
            this.btn_cancel = new System.Windows.Forms.Button();
            this.lbl_status = new System.Windows.Forms.Label();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.text_edit = new System.Windows.Forms.TextBox();
            this.btn_updateQuery = new System.Windows.Forms.Button();
            this.label4 = new System.Windows.Forms.Label();
            this.lbl_chapt = new System.Windows.Forms.Label();
            this.lbl_title = new System.Windows.Forms.Label();
            this.lbl_aEnc = new System.Windows.Forms.Label();
            this.lbl_vEnc = new System.Windows.Forms.Label();
            this.lbl_dest = new System.Windows.Forms.Label();
            this.lbl_source = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // btn_down
            // 
            this.btn_down.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_down.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_down.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_down.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_down.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_down.Location = new System.Drawing.Point(74, 274);
            this.btn_down.Name = "btn_down";
            this.btn_down.Size = new System.Drawing.Size(64, 22);
            this.btn_down.TabIndex = 33;
            this.btn_down.TabStop = false;
            this.btn_down.Text = "Down";
            this.btn_down.UseVisualStyleBackColor = false;
            this.btn_down.Click += new System.EventHandler(this.btn_down_Click);
            // 
            // btn_up
            // 
            this.btn_up.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_up.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_up.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_up.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_up.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_up.Location = new System.Drawing.Point(13, 274);
            this.btn_up.Name = "btn_up";
            this.btn_up.Size = new System.Drawing.Size(55, 22);
            this.btn_up.TabIndex = 32;
            this.btn_up.TabStop = false;
            this.btn_up.Text = "Up";
            this.btn_up.UseVisualStyleBackColor = false;
            this.btn_up.Click += new System.EventHandler(this.btn_up_Click);
            // 
            // btn_delete
            // 
            this.btn_delete.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_delete.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_delete.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_delete.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_delete.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_delete.Location = new System.Drawing.Point(144, 274);
            this.btn_delete.Name = "btn_delete";
            this.btn_delete.Size = new System.Drawing.Size(64, 22);
            this.btn_delete.TabIndex = 31;
            this.btn_delete.TabStop = false;
            this.btn_delete.Text = "Delete";
            this.btn_delete.UseVisualStyleBackColor = false;
            this.btn_delete.Click += new System.EventHandler(this.btn_delete_Click);
            // 
            // btn_q_encoder
            // 
            this.btn_q_encoder.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_q_encoder.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_q_encoder.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_q_encoder.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_q_encoder.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_q_encoder.Location = new System.Drawing.Point(12, 345);
            this.btn_q_encoder.Name = "btn_q_encoder";
            this.btn_q_encoder.Size = new System.Drawing.Size(124, 22);
            this.btn_q_encoder.TabIndex = 29;
            this.btn_q_encoder.TabStop = false;
            this.btn_q_encoder.Text = "Encode Video(s)";
            this.btn_q_encoder.UseVisualStyleBackColor = false;
            this.btn_q_encoder.Click += new System.EventHandler(this.btn_q_encoder_Click);
            // 
            // list_queue
            // 
            this.list_queue.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.list_queue.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.list_queue.HorizontalScrollbar = true;
            this.list_queue.Location = new System.Drawing.Point(12, 110);
            this.list_queue.Name = "list_queue";
            this.list_queue.Size = new System.Drawing.Size(701, 158);
            this.list_queue.TabIndex = 28;
            this.list_queue.SelectedIndexChanged += new System.EventHandler(this.list_queue_SelectedIndexChanged);
            // 
            // btn_Close
            // 
            this.btn_Close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_Close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_Close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_Close.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_Close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_Close.Location = new System.Drawing.Point(606, 345);
            this.btn_Close.Name = "btn_Close";
            this.btn_Close.Size = new System.Drawing.Size(108, 22);
            this.btn_Close.TabIndex = 27;
            this.btn_Close.TabStop = false;
            this.btn_Close.Text = "Close Window";
            this.btn_Close.UseVisualStyleBackColor = false;
            this.btn_Close.Click += new System.EventHandler(this.btn_Close_Click);
            // 
            // progressBar
            // 
            this.progressBar.BackColor = System.Drawing.SystemColors.ControlLight;
            this.progressBar.Location = new System.Drawing.Point(67, 307);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(608, 23);
            this.progressBar.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            this.progressBar.TabIndex = 34;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(10, 313);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(51, 13);
            this.label2.TabIndex = 35;
            this.label2.Text = "Progress:";
            // 
            // lbl_progressValue
            // 
            this.lbl_progressValue.AutoSize = true;
            this.lbl_progressValue.Location = new System.Drawing.Point(690, 313);
            this.lbl_progressValue.Name = "lbl_progressValue";
            this.lbl_progressValue.Size = new System.Drawing.Size(24, 13);
            this.lbl_progressValue.TabIndex = 36;
            this.lbl_progressValue.Text = "0 %";
            // 
            // btn_cancel
            // 
            this.btn_cancel.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_cancel.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_cancel.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_cancel.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_cancel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_cancel.Location = new System.Drawing.Point(143, 345);
            this.btn_cancel.Name = "btn_cancel";
            this.btn_cancel.Size = new System.Drawing.Size(91, 22);
            this.btn_cancel.TabIndex = 41;
            this.btn_cancel.TabStop = false;
            this.btn_cancel.Text = "Stop Queue";
            this.toolTip1.SetToolTip(this.btn_cancel, "This will prevent any more encode processes from starting. It will not stop the c" +
                    "urrent process!");
            this.btn_cancel.UseVisualStyleBackColor = false;
            this.btn_cancel.Visible = false;
            this.btn_cancel.Click += new System.EventHandler(this.btn_cancel_Click);
            // 
            // lbl_status
            // 
            this.lbl_status.AutoSize = true;
            this.lbl_status.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_status.Location = new System.Drawing.Point(274, 313);
            this.lbl_status.Name = "lbl_status";
            this.lbl_status.Size = new System.Drawing.Size(176, 13);
            this.lbl_status.TabIndex = 42;
            this.lbl_status.Text = "Encode Queue Completed!";
            this.lbl_status.Visible = false;
            // 
            // text_edit
            // 
            this.text_edit.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.text_edit.Location = new System.Drawing.Point(214, 275);
            this.text_edit.Name = "text_edit";
            this.text_edit.Size = new System.Drawing.Size(430, 20);
            this.text_edit.TabIndex = 43;
            this.toolTip1.SetToolTip(this.text_edit, "Avoid using this feature when the encoder is about to start a new task. You may o" +
                    "verwrite the wrong job");
            // 
            // btn_updateQuery
            // 
            this.btn_updateQuery.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_updateQuery.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_updateQuery.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_updateQuery.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_updateQuery.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_updateQuery.Location = new System.Drawing.Point(650, 274);
            this.btn_updateQuery.Name = "btn_updateQuery";
            this.btn_updateQuery.Size = new System.Drawing.Size(64, 23);
            this.btn_updateQuery.TabIndex = 46;
            this.btn_updateQuery.TabStop = false;
            this.btn_updateQuery.Text = "Update";
            this.btn_updateQuery.UseVisualStyleBackColor = false;
            this.btn_updateQuery.Click += new System.EventHandler(this.btn_updateQuery_Click);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(232, 72);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(47, 26);
            this.label4.TabIndex = 70;
            this.label4.Text = "Video:\r\nAudo:";
            // 
            // lbl_chapt
            // 
            this.lbl_chapt.AutoSize = true;
            this.lbl_chapt.Location = new System.Drawing.Point(175, 84);
            this.lbl_chapt.Name = "lbl_chapt";
            this.lbl_chapt.Size = new System.Drawing.Size(10, 13);
            this.lbl_chapt.TabIndex = 69;
            this.lbl_chapt.Text = "-";
            // 
            // lbl_title
            // 
            this.lbl_title.AccessibleRole = System.Windows.Forms.AccessibleRole.None;
            this.lbl_title.AutoSize = true;
            this.lbl_title.Location = new System.Drawing.Point(175, 71);
            this.lbl_title.Name = "lbl_title";
            this.lbl_title.Size = new System.Drawing.Size(10, 13);
            this.lbl_title.TabIndex = 68;
            this.lbl_title.Text = "-";
            // 
            // lbl_aEnc
            // 
            this.lbl_aEnc.AutoSize = true;
            this.lbl_aEnc.Location = new System.Drawing.Point(285, 85);
            this.lbl_aEnc.Name = "lbl_aEnc";
            this.lbl_aEnc.Size = new System.Drawing.Size(10, 13);
            this.lbl_aEnc.TabIndex = 67;
            this.lbl_aEnc.Text = "-";
            // 
            // lbl_vEnc
            // 
            this.lbl_vEnc.AutoSize = true;
            this.lbl_vEnc.Location = new System.Drawing.Point(285, 72);
            this.lbl_vEnc.Name = "lbl_vEnc";
            this.lbl_vEnc.Size = new System.Drawing.Size(10, 13);
            this.lbl_vEnc.TabIndex = 66;
            this.lbl_vEnc.Text = "-";
            // 
            // lbl_dest
            // 
            this.lbl_dest.AutoSize = true;
            this.lbl_dest.Location = new System.Drawing.Point(175, 59);
            this.lbl_dest.Name = "lbl_dest";
            this.lbl_dest.Size = new System.Drawing.Size(10, 13);
            this.lbl_dest.TabIndex = 65;
            this.lbl_dest.Text = "-";
            this.lbl_dest.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lbl_source
            // 
            this.lbl_source.AutoSize = true;
            this.lbl_source.Location = new System.Drawing.Point(175, 46);
            this.lbl_source.Name = "lbl_source";
            this.lbl_source.Size = new System.Drawing.Size(10, 13);
            this.lbl_source.TabIndex = 64;
            this.lbl_source.Text = "-";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(50, 46);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(99, 52);
            this.label1.TabIndex = 63;
            this.label1.Text = "Source:\r\nDestination:\r\nDVD Title:\r\nDVD Chapters:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(50, 24);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(82, 13);
            this.label3.TabIndex = 62;
            this.label3.Text = "Current Job";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = global::Handbrake.Properties.Resources.Queue;
            this.pictureBox1.Location = new System.Drawing.Point(12, 12);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(32, 32);
            this.pictureBox1.TabIndex = 61;
            this.pictureBox1.TabStop = false;
            // 
            // frmQueue
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ClientSize = new System.Drawing.Size(729, 377);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.lbl_chapt);
            this.Controls.Add(this.lbl_title);
            this.Controls.Add(this.lbl_aEnc);
            this.Controls.Add(this.lbl_vEnc);
            this.Controls.Add(this.lbl_dest);
            this.Controls.Add(this.lbl_source);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.list_queue);
            this.Controls.Add(this.btn_updateQuery);
            this.Controls.Add(this.btn_delete);
            this.Controls.Add(this.lbl_status);
            this.Controls.Add(this.btn_cancel);
            this.Controls.Add(this.btn_up);
            this.Controls.Add(this.text_edit);
            this.Controls.Add(this.btn_down);
            this.Controls.Add(this.lbl_progressValue);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.btn_q_encoder);
            this.Controls.Add(this.btn_Close);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "frmQueue";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Encode Queue";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Button btn_down;
        internal System.Windows.Forms.Button btn_up;
        internal System.Windows.Forms.Button btn_delete;
        internal System.Windows.Forms.Button btn_q_encoder;
        internal System.Windows.Forms.Button btn_Close;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label lbl_progressValue;
        internal System.Windows.Forms.Button btn_cancel;
        private System.Windows.Forms.Label lbl_status;
        private System.Windows.Forms.ToolTip toolTip1;
        public System.Windows.Forms.ListBox list_queue;
        private System.Windows.Forms.TextBox text_edit;
        internal System.Windows.Forms.Button btn_updateQuery;
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
    }
}