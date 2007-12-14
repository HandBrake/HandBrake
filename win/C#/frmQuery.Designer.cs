namespace Handbrake
{
    partial class frmQuery
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmQuery));
            this.rtf_query = new System.Windows.Forms.RichTextBox();
            this.btn_close = new System.Windows.Forms.Button();
            this.btn_copy = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // rtf_query
            // 
            this.rtf_query.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.rtf_query.Location = new System.Drawing.Point(12, 12);
            this.rtf_query.Name = "rtf_query";
            this.rtf_query.Size = new System.Drawing.Size(543, 102);
            this.rtf_query.TabIndex = 0;
            this.rtf_query.Text = "";
            // 
            // btn_close
            // 
            this.btn_close.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_close.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_close.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_close.Location = new System.Drawing.Point(456, 120);
            this.btn_close.Name = "btn_close";
            this.btn_close.Size = new System.Drawing.Size(99, 22);
            this.btn_close.TabIndex = 28;
            this.btn_close.Text = "Close";
            this.btn_close.UseVisualStyleBackColor = false;
            this.btn_close.Click += new System.EventHandler(this.btn_close_Click);
            // 
            // btn_copy
            // 
            this.btn_copy.BackColor = System.Drawing.SystemColors.ControlLight;
            this.btn_copy.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_copy.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.btn_copy.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_copy.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_copy.Location = new System.Drawing.Point(12, 120);
            this.btn_copy.Name = "btn_copy";
            this.btn_copy.Size = new System.Drawing.Size(134, 23);
            this.btn_copy.TabIndex = 29;
            this.btn_copy.Text = "Copy to Clipboard";
            this.btn_copy.UseVisualStyleBackColor = false;
            this.btn_copy.Click += new System.EventHandler(this.btn_copy_Click);
            // 
            // frmQuery
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(564, 150);
            this.Controls.Add(this.btn_copy);
            this.Controls.Add(this.btn_close);
            this.Controls.Add(this.rtf_query);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmQuery";
            this.Text = "The CLI Query";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.RichTextBox rtf_query;
        internal System.Windows.Forms.Button btn_close;
        internal System.Windows.Forms.Button btn_copy;
    }
}