namespace Handbrake
{
    partial class frmPreview
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
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmPreview));
            this.startPoint = new System.Windows.Forms.ComboBox();
            this.endPoint = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.lbl_progress = new System.Windows.Forms.Label();
            this.btn_play = new System.Windows.Forms.Button();
            this.defaultPlayer = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // startPoint
            // 
            this.startPoint.FormattingEnabled = true;
            this.startPoint.Items.AddRange(new object[] {
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10"});
            this.startPoint.Location = new System.Drawing.Point(107, 6);
            this.startPoint.Name = "startPoint";
            this.startPoint.Size = new System.Drawing.Size(72, 21);
            this.startPoint.TabIndex = 38;
            // 
            // endPoint
            // 
            this.endPoint.FormattingEnabled = true;
            this.endPoint.Items.AddRange(new object[] {
            "10",
            "30",
            "45",
            "60",
            "75",
            "90",
            "105",
            "120"});
            this.endPoint.Location = new System.Drawing.Point(289, 6);
            this.endPoint.Name = "endPoint";
            this.endPoint.Size = new System.Drawing.Size(70, 21);
            this.endPoint.TabIndex = 39;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(89, 13);
            this.label1.TabIndex = 40;
            this.label1.Text = "Start at Preview:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(185, 9);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(98, 13);
            this.label2.TabIndex = 41;
            this.label2.Text = "Duration (seconds)";
            // 
            // progressBar
            // 
            this.progressBar.Location = new System.Drawing.Point(12, 33);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(301, 18);
            this.progressBar.TabIndex = 42;
            // 
            // lbl_progress
            // 
            this.lbl_progress.AutoSize = true;
            this.lbl_progress.Location = new System.Drawing.Point(319, 36);
            this.lbl_progress.Name = "lbl_progress";
            this.lbl_progress.Size = new System.Drawing.Size(40, 13);
            this.lbl_progress.TabIndex = 43;
            this.lbl_progress.Text = "0.00%";
            // 
            // btn_play
            // 
            this.btn_play.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_play.ForeColor = System.Drawing.Color.DarkOrange;
            this.btn_play.Location = new System.Drawing.Point(302, 57);
            this.btn_play.Name = "btn_play";
            this.btn_play.Size = new System.Drawing.Size(57, 24);
            this.btn_play.TabIndex = 44;
            this.btn_play.Text = "Play";
            this.btn_play.UseVisualStyleBackColor = true;
            this.btn_play.Click += new System.EventHandler(this.btn_play_Click);
            // 
            // defaultPlayer
            // 
            this.defaultPlayer.AutoSize = true;
            this.defaultPlayer.Location = new System.Drawing.Point(12, 62);
            this.defaultPlayer.Name = "defaultPlayer";
            this.defaultPlayer.Size = new System.Drawing.Size(151, 17);
            this.defaultPlayer.TabIndex = 45;
            this.defaultPlayer.Text = "Use system default player";
            this.defaultPlayer.UseVisualStyleBackColor = true;
            this.defaultPlayer.CheckedChanged += new System.EventHandler(this.DefaultPlayerCheckedChanged);
            // 
            // frmPreview
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(369, 91);
            this.Controls.Add(this.defaultPlayer);
            this.Controls.Add(this.btn_play);
            this.Controls.Add(this.lbl_progress);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.endPoint);
            this.Controls.Add(this.startPoint);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmPreview";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Video Preview";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ComboBox startPoint;
        private System.Windows.Forms.ComboBox endPoint;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.Label lbl_progress;
        private System.Windows.Forms.Button btn_play;
        private System.Windows.Forms.CheckBox defaultPlayer;

    }
}