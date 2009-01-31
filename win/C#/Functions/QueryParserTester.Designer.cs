namespace Handbrake.Functions
{
    partial class QueryParserTester
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
            this.rtf_testContent = new System.Windows.Forms.RichTextBox();
            this.btn_test = new System.Windows.Forms.Button();
            this.rtf_query = new System.Windows.Forms.RichTextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // rtf_testContent
            // 
            this.rtf_testContent.Location = new System.Drawing.Point(111, 164);
            this.rtf_testContent.Name = "rtf_testContent";
            this.rtf_testContent.Size = new System.Drawing.Size(641, 420);
            this.rtf_testContent.TabIndex = 0;
            this.rtf_testContent.Text = "";
            // 
            // btn_test
            // 
            this.btn_test.Location = new System.Drawing.Point(405, 115);
            this.btn_test.Name = "btn_test";
            this.btn_test.Size = new System.Drawing.Size(75, 23);
            this.btn_test.TabIndex = 1;
            this.btn_test.Text = "Test";
            this.btn_test.UseVisualStyleBackColor = true;
            this.btn_test.Click += new System.EventHandler(this.btn_test_Click);
            // 
            // rtf_query
            // 
            this.rtf_query.Location = new System.Drawing.Point(111, 12);
            this.rtf_query.Name = "rtf_query";
            this.rtf_query.Size = new System.Drawing.Size(641, 77);
            this.rtf_query.TabIndex = 2;
            this.rtf_query.Text = "";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(37, 43);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(38, 13);
            this.label1.TabIndex = 3;
            this.label1.Text = "Query:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(37, 327);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(40, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Result:";
            // 
            // QueryParserTester
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(780, 625);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.rtf_query);
            this.Controls.Add(this.btn_test);
            this.Controls.Add(this.rtf_testContent);
            this.Name = "QueryParserTester";
            this.Text = "QueryParserTester";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.RichTextBox rtf_testContent;
        private System.Windows.Forms.Button btn_test;
        private System.Windows.Forms.RichTextBox rtf_query;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
    }
}