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
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.lbl_dest = new System.Windows.Forms.Label();
            this.lbl_source = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.btn_encode = new System.Windows.Forms.ToolStripButton();
            this.btn_pause = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.drop_button_queue = new System.Windows.Forms.ToolStripDropDownButton();
            this.mnu_batch = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_import = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_export = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_readd = new System.Windows.Forms.ToolStripMenuItem();
            this.drp_completeOption = new System.Windows.Forms.ToolStripComboBox();
            this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
            this.SaveFile = new System.Windows.Forms.SaveFileDialog();
            this.list_queue = new System.Windows.Forms.ListView();
            this.Title = new System.Windows.Forms.ColumnHeader();
            this.Chapters = new System.Windows.Forms.ColumnHeader();
            this.Source = new System.Windows.Forms.ColumnHeader();
            this.Destination = new System.Windows.Forms.ColumnHeader();
            this.EncoderVideo = new System.Windows.Forms.ColumnHeader();
            this.Audio = new System.Windows.Forms.ColumnHeader();
            this.mnu_queue = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_up = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_Down = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_edit = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_delete = new System.Windows.Forms.ToolStripMenuItem();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.lbl_encodesPending = new System.Windows.Forms.ToolStripStatusLabel();
            this.OpenFile = new System.Windows.Forms.OpenFileDialog();
            this.lbl_encodeStatus = new System.Windows.Forms.Label();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.lbl_encodeOptions = new System.Windows.Forms.Label();
            this.panel2 = new System.Windows.Forms.Panel();
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel3 = new System.Windows.Forms.Panel();
            this.toolStrip1.SuspendLayout();
            this.mnu_queue.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // lbl_dest
            // 
            this.lbl_dest.AutoEllipsis = true;
            this.lbl_dest.Location = new System.Drawing.Point(117, 60);
            this.lbl_dest.Name = "lbl_dest";
            this.lbl_dest.Size = new System.Drawing.Size(671, 13);
            this.lbl_dest.TabIndex = 65;
            this.lbl_dest.Text = "-";
            this.lbl_dest.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // lbl_source
            // 
            this.lbl_source.AutoEllipsis = true;
            this.lbl_source.Location = new System.Drawing.Point(117, 47);
            this.lbl_source.Name = "lbl_source";
            this.lbl_source.Size = new System.Drawing.Size(671, 13);
            this.lbl_source.TabIndex = 64;
            this.lbl_source.Text = "-";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(12, 47);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(85, 39);
            this.label1.TabIndex = 63;
            this.label1.Text = "Source:\r\nDestination:\r\nOptions:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(12, 9);
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
            this.btn_pause,
            this.toolStripSeparator1,
            this.drop_button_queue,
            this.drp_completeOption,
            this.toolStripButton1});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.toolStrip1.Size = new System.Drawing.Size(789, 39);
            this.toolStrip1.TabIndex = 71;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // btn_encode
            // 
            this.btn_encode.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_encode.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_encode.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_encode.Name = "btn_encode";
            this.btn_encode.Size = new System.Drawing.Size(82, 36);
            this.btn_encode.Text = "Encode";
            this.btn_encode.Click += new System.EventHandler(this.BtnEncodeClick);
            // 
            // btn_pause
            // 
            this.btn_pause.Image = global::Handbrake.Properties.Resources.Pause;
            this.btn_pause.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_pause.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_pause.Name = "btn_pause";
            this.btn_pause.Overflow = System.Windows.Forms.ToolStripItemOverflow.Never;
            this.btn_pause.Size = new System.Drawing.Size(74, 36);
            this.btn_pause.Text = "Pause";
            this.btn_pause.Visible = false;
            this.btn_pause.Click += new System.EventHandler(this.BtnPauseClick);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 39);
            // 
            // drop_button_queue
            // 
            this.drop_button_queue.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_batch,
            this.mnu_import,
            this.mnu_export,
            this.toolStripSeparator2,
            this.mnu_readd});
            this.drop_button_queue.Image = global::Handbrake.Properties.Resources.ActivityWindow;
            this.drop_button_queue.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.drop_button_queue.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.drop_button_queue.Name = "drop_button_queue";
            this.drop_button_queue.Size = new System.Drawing.Size(87, 36);
            this.drop_button_queue.Text = "Queue";
            // 
            // mnu_batch
            // 
            this.mnu_batch.Image = global::Handbrake.Properties.Resources.Output_Small;
            this.mnu_batch.Name = "mnu_batch";
            this.mnu_batch.Size = new System.Drawing.Size(235, 22);
            this.mnu_batch.Text = "Generate Batch Script";
            this.mnu_batch.Click += new System.EventHandler(this.MnuBatchClick);
            // 
            // mnu_import
            // 
            this.mnu_import.Image = global::Handbrake.Properties.Resources.folder;
            this.mnu_import.Name = "mnu_import";
            this.mnu_import.Size = new System.Drawing.Size(235, 22);
            this.mnu_import.Text = "Import Queue";
            this.mnu_import.Click += new System.EventHandler(this.MnuImportClick);
            // 
            // mnu_export
            // 
            this.mnu_export.Image = global::Handbrake.Properties.Resources.save;
            this.mnu_export.Name = "mnu_export";
            this.mnu_export.Size = new System.Drawing.Size(235, 22);
            this.mnu_export.Text = "Export Queue";
            this.mnu_export.Click += new System.EventHandler(this.MnuExportClick);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(232, 6);
            // 
            // mnu_readd
            // 
            this.mnu_readd.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.mnu_readd.Image = global::Handbrake.Properties.Resources.AddToQueue_small;
            this.mnu_readd.Name = "mnu_readd";
            this.mnu_readd.Size = new System.Drawing.Size(235, 22);
            this.mnu_readd.Text = "Re-Add Currently Running Job";
            this.mnu_readd.ToolTipText = "Readds the currently encoding job back onto the queue.";
            this.mnu_readd.Click += new System.EventHandler(this.MnuReaddClick);
            // 
            // drp_completeOption
            // 
            this.drp_completeOption.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.drp_completeOption.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_completeOption.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.drp_completeOption.Items.AddRange(new object[] {
            "Do nothing",
            "Shutdown",
            "Suspend",
            "Hibernate",
            "Lock system",
            "Log off",
            "Quit HandBrake"});
            this.drp_completeOption.Margin = new System.Windows.Forms.Padding(1, 0, 15, 0);
            this.drp_completeOption.Name = "drp_completeOption";
            this.drp_completeOption.Size = new System.Drawing.Size(155, 39);
            this.drp_completeOption.SelectedIndexChanged += new System.EventHandler(this.CompleteOptionChanged);
            // 
            // toolStripButton1
            // 
            this.toolStripButton1.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.toolStripButton1.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.toolStripButton1.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButton1.Image")));
            this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButton1.Name = "toolStripButton1";
            this.toolStripButton1.Size = new System.Drawing.Size(76, 36);
            this.toolStripButton1.Text = "When Done:";
            // 
            // SaveFile
            // 
            this.SaveFile.Filter = "Batch|.bat";
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
            this.list_queue.ContextMenuStrip = this.mnu_queue;
            this.list_queue.Dock = System.Windows.Forms.DockStyle.Fill;
            this.list_queue.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.list_queue.FullRowSelect = true;
            this.list_queue.GridLines = true;
            this.list_queue.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.list_queue.Location = new System.Drawing.Point(15, 0);
            this.list_queue.Name = "list_queue";
            this.list_queue.Size = new System.Drawing.Size(759, 199);
            this.list_queue.TabIndex = 72;
            this.list_queue.UseCompatibleStateImageBehavior = false;
            this.list_queue.View = System.Windows.Forms.View.Details;
            this.list_queue.KeyUp += new System.Windows.Forms.KeyEventHandler(this.ListQueueDeleteKey);
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
            this.Destination.Width = 210;
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
            // mnu_queue
            // 
            this.mnu_queue.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_up,
            this.mnu_Down,
            this.toolStripSeparator3,
            this.mnu_edit,
            this.toolStripSeparator4,
            this.mnu_delete});
            this.mnu_queue.Name = "mnu_queue";
            this.mnu_queue.Size = new System.Drawing.Size(139, 104);
            // 
            // mnu_up
            // 
            this.mnu_up.Name = "mnu_up";
            this.mnu_up.Size = new System.Drawing.Size(138, 22);
            this.mnu_up.Text = "Move Up";
            this.mnu_up.Click += new System.EventHandler(this.MnuUpClick);
            // 
            // mnu_Down
            // 
            this.mnu_Down.Name = "mnu_Down";
            this.mnu_Down.Size = new System.Drawing.Size(138, 22);
            this.mnu_Down.Text = "Move Down";
            this.mnu_Down.Click += new System.EventHandler(this.MnuDownClick);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(135, 6);
            // 
            // mnu_edit
            // 
            this.mnu_edit.Name = "mnu_edit";
            this.mnu_edit.Size = new System.Drawing.Size(138, 22);
            this.mnu_edit.Text = "Edit";
            this.mnu_edit.Click += new System.EventHandler(this.MnuEditClick);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(135, 6);
            // 
            // mnu_delete
            // 
            this.mnu_delete.Name = "mnu_delete";
            this.mnu_delete.Size = new System.Drawing.Size(138, 22);
            this.mnu_delete.Text = "Delete";
            this.mnu_delete.Click += new System.EventHandler(this.MnuDeleteClick);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.lbl_encodesPending});
            this.statusStrip1.Location = new System.Drawing.Point(0, 363);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Padding = new System.Windows.Forms.Padding(1, 0, 12, 0);
            this.statusStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.statusStrip1.Size = new System.Drawing.Size(789, 22);
            this.statusStrip1.TabIndex = 73;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // lbl_encodesPending
            // 
            this.lbl_encodesPending.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.lbl_encodesPending.Margin = new System.Windows.Forms.Padding(0, 3, 10, 2);
            this.lbl_encodesPending.Name = "lbl_encodesPending";
            this.lbl_encodesPending.Size = new System.Drawing.Size(115, 17);
            this.lbl_encodesPending.Text = "0 encode(s) pending";
            // 
            // OpenFile
            // 
            this.OpenFile.Filter = "HandBrake Queue|*.queue";
            // 
            // lbl_encodeStatus
            // 
            this.lbl_encodeStatus.AutoSize = true;
            this.lbl_encodeStatus.Location = new System.Drawing.Point(12, 30);
            this.lbl_encodeStatus.Name = "lbl_encodeStatus";
            this.lbl_encodeStatus.Size = new System.Drawing.Size(38, 13);
            this.lbl_encodeStatus.TabIndex = 73;
            this.lbl_encodeStatus.Text = "Ready";
            // 
            // splitContainer1
            // 
            this.splitContainer1.BackColor = System.Drawing.Color.Transparent;
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.Location = new System.Drawing.Point(0, 39);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.BackColor = System.Drawing.Color.Transparent;
            this.splitContainer1.Panel1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.splitContainer1.Panel1.Controls.Add(this.lbl_encodeOptions);
            this.splitContainer1.Panel1.Controls.Add(this.label3);
            this.splitContainer1.Panel1.Controls.Add(this.lbl_dest);
            this.splitContainer1.Panel1.Controls.Add(this.label1);
            this.splitContainer1.Panel1.Controls.Add(this.lbl_encodeStatus);
            this.splitContainer1.Panel1.Controls.Add(this.lbl_source);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.list_queue);
            this.splitContainer1.Panel2.Controls.Add(this.panel3);
            this.splitContainer1.Panel2.Controls.Add(this.panel2);
            this.splitContainer1.Panel2.Controls.Add(this.panel1);
            this.splitContainer1.Size = new System.Drawing.Size(789, 324);
            this.splitContainer1.SplitterDistance = 106;
            this.splitContainer1.TabIndex = 74;
            // 
            // lbl_encodeOptions
            // 
            this.lbl_encodeOptions.AutoEllipsis = true;
            this.lbl_encodeOptions.Location = new System.Drawing.Point(117, 73);
            this.lbl_encodeOptions.Name = "lbl_encodeOptions";
            this.lbl_encodeOptions.Size = new System.Drawing.Size(671, 33);
            this.lbl_encodeOptions.TabIndex = 74;
            this.lbl_encodeOptions.Text = "-";
            // 
            // panel2
            // 
            this.panel2.BackColor = System.Drawing.Color.Transparent;
            this.panel2.Dock = System.Windows.Forms.DockStyle.Right;
            this.panel2.Location = new System.Drawing.Point(774, 0);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(15, 214);
            this.panel2.TabIndex = 1;
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.Transparent;
            this.panel1.Dock = System.Windows.Forms.DockStyle.Left;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(15, 214);
            this.panel1.TabIndex = 0;
            // 
            // panel3
            // 
            this.panel3.BackColor = System.Drawing.Color.Transparent;
            this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel3.Location = new System.Drawing.Point(15, 199);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(759, 15);
            this.panel3.TabIndex = 73;
            // 
            // frmQueue
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(789, 385);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.toolStrip1);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(677, 417);
            this.Name = "frmQueue";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Encode Queue";
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.mnu_queue.ResumeLayout(false);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.Label lbl_dest;
        private System.Windows.Forms.Label lbl_source;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton btn_encode;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton btn_pause;
        private System.Windows.Forms.SaveFileDialog SaveFile;
        private System.Windows.Forms.ListView list_queue;
        private System.Windows.Forms.ColumnHeader Title;
        private System.Windows.Forms.ColumnHeader Chapters;
        private System.Windows.Forms.ColumnHeader Source;
        private System.Windows.Forms.ColumnHeader Destination;
        private System.Windows.Forms.ColumnHeader EncoderVideo;
        private System.Windows.Forms.ColumnHeader Audio;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripDropDownButton drop_button_queue;
        private System.Windows.Forms.ToolStripMenuItem mnu_batch;
        private System.Windows.Forms.ToolStripMenuItem mnu_import;
        private System.Windows.Forms.ToolStripMenuItem mnu_export;
        private System.Windows.Forms.OpenFileDialog OpenFile;
        private System.Windows.Forms.ToolStripStatusLabel lbl_encodesPending;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem mnu_readd;
        private System.Windows.Forms.ContextMenuStrip mnu_queue;
        private System.Windows.Forms.ToolStripMenuItem mnu_up;
        private System.Windows.Forms.ToolStripMenuItem mnu_Down;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripMenuItem mnu_delete;
        private System.Windows.Forms.ToolStripMenuItem mnu_edit;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.Label lbl_encodeStatus;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Label lbl_encodeOptions;
        private System.Windows.Forms.ToolStripComboBox drp_completeOption;
        private System.Windows.Forms.ToolStripButton toolStripButton1;
        private System.Windows.Forms.Panel panel3;
    }
}
