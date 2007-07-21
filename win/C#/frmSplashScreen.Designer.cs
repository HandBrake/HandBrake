namespace Handbrake
{
    partial class frmSplashScreen
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmSplashScreen));
            this.productName = new System.Windows.Forms.Label();
            this.cliVersion = new System.Windows.Forms.Label();
            this.guiVerison = new System.Windows.Forms.Label();
            this.lbl_cli = new System.Windows.Forms.Label();
            this.lbl_gui = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // productName
            // 
            this.productName.AutoSize = true;
            this.productName.Font = new System.Drawing.Font("Verdana", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.productName.Location = new System.Drawing.Point(177, 69);
            this.productName.Name = "productName";
            this.productName.Size = new System.Drawing.Size(180, 32);
            this.productName.TabIndex = 0;
            this.productName.Text = "Handbrake";
            // 
            // cliVersion
            // 
            this.cliVersion.AutoSize = true;
            this.cliVersion.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.cliVersion.Location = new System.Drawing.Point(180, 197);
            this.cliVersion.Name = "cliVersion";
            this.cliVersion.Size = new System.Drawing.Size(145, 13);
            this.cliVersion.TabIndex = 1;
            this.cliVersion.Text = "Handbrake CLI Version:";
            // 
            // guiVerison
            // 
            this.guiVerison.AutoSize = true;
            this.guiVerison.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.guiVerison.Location = new System.Drawing.Point(180, 170);
            this.guiVerison.Name = "guiVerison";
            this.guiVerison.Size = new System.Drawing.Size(55, 13);
            this.guiVerison.TabIndex = 2;
            this.guiVerison.Text = "Version:";
            // 
            // lbl_cli
            // 
            this.lbl_cli.AutoSize = true;
            this.lbl_cli.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_cli.Location = new System.Drawing.Point(331, 197);
            this.lbl_cli.Name = "lbl_cli";
            this.lbl_cli.Size = new System.Drawing.Size(88, 13);
            this.lbl_cli.TabIndex = 4;
            this.lbl_cli.Text = "{CLI Version}";
            // 
            // lbl_gui
            // 
            this.lbl_gui.AutoSize = true;
            this.lbl_gui.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_gui.Location = new System.Drawing.Point(331, 170);
            this.lbl_gui.Name = "lbl_gui";
            this.lbl_gui.Size = new System.Drawing.Size(90, 13);
            this.lbl_gui.TabIndex = 5;
            this.lbl_gui.Text = "{GUI Version}";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(418, 170);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(43, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "(Beta)";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(418, 197);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(43, 13);
            this.label4.TabIndex = 7;
            this.label4.Text = "(Beta)";
            // 
            // frmSplashScreen
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.White;
            this.BackgroundImage = global::Handbrake.Properties.Resources.splash;
            this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.ClientSize = new System.Drawing.Size(496, 276);
            this.ControlBox = false;
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lbl_gui);
            this.Controls.Add(this.lbl_cli);
            this.Controls.Add(this.guiVerison);
            this.Controls.Add(this.cliVersion);
            this.Controls.Add(this.productName);
            this.Font = new System.Drawing.Font("Corbel", 8.25F);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "frmSplashScreen";
            this.Text = "SplashScreen";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label productName;
        private System.Windows.Forms.Label cliVersion;
        private System.Windows.Forms.Label guiVerison;
        private System.Windows.Forms.Label lbl_cli;
        private System.Windows.Forms.Label lbl_gui;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label4;
    }
}