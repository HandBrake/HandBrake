namespace Handbrake.ToolWindows
{
    partial class BatchAdd
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(BatchAdd));
            this.btnAdd = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.maxDuration = new System.Windows.Forms.MaskedTextBox();
            this.minDuration = new System.Windows.Forms.MaskedTextBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.label37 = new System.Windows.Forms.Label();
            this.panel2 = new System.Windows.Forms.Panel();
            this.label8 = new System.Windows.Forms.Label();
            this.pictureBox2 = new System.Windows.Forms.PictureBox();
            this.lbl_display = new System.Windows.Forms.Label();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
            this.SuspendLayout();
            // 
            // btnAdd
            // 
            this.btnAdd.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnAdd.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btnAdd.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnAdd.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btnAdd.Location = new System.Drawing.Point(194, 117);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(90, 22);
            this.btnAdd.TabIndex = 4;
            this.btnAdd.TabStop = false;
            this.btnAdd.Text = "Add Titles";
            this.btnAdd.UseVisualStyleBackColor = true;
            this.btnAdd.Click += new System.EventHandler(this.BtnScanClick);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btnCancel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnCancel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btnCancel.Location = new System.Drawing.Point(128, 117);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(60, 22);
            this.btnCancel.TabIndex = 5;
            this.btnCancel.TabStop = false;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.BtnCancelClick);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(125, 74);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(25, 13);
            this.label3.TabIndex = 7;
            this.label3.Text = "and";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(227, 74);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(57, 13);
            this.label4.TabIndex = 8;
            this.label4.Text = "(hh:mm:ss)";
            // 
            // maxDuration
            // 
            this.maxDuration.Location = new System.Drawing.Point(156, 71);
            this.maxDuration.Mask = "00:00:00";
            this.maxDuration.Name = "maxDuration";
            this.maxDuration.RejectInputOnFirstFailure = true;
            this.maxDuration.Size = new System.Drawing.Size(65, 20);
            this.maxDuration.TabIndex = 11;
            this.maxDuration.Text = "023000";
            this.maxDuration.TextChanged += new System.EventHandler(this.maxDuration_TextChanged);
            // 
            // minDuration
            // 
            this.minDuration.Location = new System.Drawing.Point(54, 71);
            this.minDuration.Mask = "00:00:00";
            this.minDuration.Name = "minDuration";
            this.minDuration.Size = new System.Drawing.Size(65, 20);
            this.minDuration.TabIndex = 10;
            this.minDuration.Text = "001800";
            this.minDuration.TextChanged += new System.EventHandler(this.minDuration_TextChanged);
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.White;
            this.panel1.Controls.Add(this.label37);
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Controls.Add(this.label8);
            this.panel1.Controls.Add(this.pictureBox2);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Margin = new System.Windows.Forms.Padding(0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(299, 65);
            this.panel1.TabIndex = 63;
            // 
            // label37
            // 
            this.label37.AutoSize = true;
            this.label37.Location = new System.Drawing.Point(51, 27);
            this.label37.Margin = new System.Windows.Forms.Padding(3);
            this.label37.Name = "label37";
            this.label37.Size = new System.Drawing.Size(166, 13);
            this.label37.TabIndex = 105;
            this.label37.Text = "Using the current encode settings";
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.SystemColors.Control;
            this.panel2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel2.Location = new System.Drawing.Point(0, 55);
            this.panel2.MaximumSize = new System.Drawing.Size(0, 10);
            this.panel2.MinimumSize = new System.Drawing.Size(0, 10);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(299, 10);
            this.panel2.TabIndex = 59;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label8.Location = new System.Drawing.Point(51, 11);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(124, 13);
            this.label8.TabIndex = 61;
            this.label8.Text = "Add all Titles betwen";
            // 
            // pictureBox2
            // 
            this.pictureBox2.Image = global::Handbrake.Properties.Resources.AddToQueue;
            this.pictureBox2.Location = new System.Drawing.Point(12, 9);
            this.pictureBox2.Margin = new System.Windows.Forms.Padding(3, 0, 3, 3);
            this.pictureBox2.Name = "pictureBox2";
            this.pictureBox2.Size = new System.Drawing.Size(32, 32);
            this.pictureBox2.TabIndex = 60;
            this.pictureBox2.TabStop = false;
            // 
            // lbl_display
            // 
            this.lbl_display.AutoSize = true;
            this.lbl_display.Location = new System.Drawing.Point(51, 96);
            this.lbl_display.Name = "lbl_display";
            this.lbl_display.Size = new System.Drawing.Size(112, 13);
            this.lbl_display.TabIndex = 64;
            this.lbl_display.Text = "This will add {0} items.";
            // 
            // BatchAdd
            // 
            this.AcceptButton = this.btnAdd;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(299, 151);
            this.Controls.Add(this.lbl_display);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.maxDuration);
            this.Controls.Add(this.minDuration);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnAdd);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "BatchAdd";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "*** THIS IS A CONCEPT WINDOW ***";
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.Button btnAdd;
        internal System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.MaskedTextBox maxDuration;
        private System.Windows.Forms.MaskedTextBox minDuration;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Label label37;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.PictureBox pictureBox2;
        private System.Windows.Forms.Label lbl_display;
    }
}