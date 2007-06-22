<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmQueue
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        If disposing AndAlso components IsNot Nothing Then
            components.Dispose()
        End If
        MyBase.Dispose(disposing)
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmQueue))
        Me.btn_Close = New System.Windows.Forms.Button
        Me.list_queue = New System.Windows.Forms.ListBox
        Me.btn_q_encoder = New System.Windows.Forms.Button
        Me.Label1 = New System.Windows.Forms.Label
        Me.btn_delete = New System.Windows.Forms.Button
        Me.SuspendLayout()
        '
        'btn_Close
        '
        Me.btn_Close.BackColor = System.Drawing.SystemColors.Control
        Me.btn_Close.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_Close.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_Close.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_Close.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_Close.Location = New System.Drawing.Point(16, 403)
        Me.btn_Close.Name = "btn_Close"
        Me.btn_Close.Size = New System.Drawing.Size(124, 22)
        Me.btn_Close.TabIndex = 20
        Me.btn_Close.TabStop = False
        Me.btn_Close.Text = "Close Window"
        Me.btn_Close.UseVisualStyleBackColor = False
        '
        'list_queue
        '
        Me.list_queue.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle
        Me.list_queue.FormattingEnabled = True
        Me.list_queue.Location = New System.Drawing.Point(16, 34)
        Me.list_queue.Name = "list_queue"
        Me.list_queue.Size = New System.Drawing.Size(593, 353)
        Me.list_queue.TabIndex = 21
        '
        'btn_q_encoder
        '
        Me.btn_q_encoder.BackColor = System.Drawing.SystemColors.Control
        Me.btn_q_encoder.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_q_encoder.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_q_encoder.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_q_encoder.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_q_encoder.Location = New System.Drawing.Point(485, 403)
        Me.btn_q_encoder.Name = "btn_q_encoder"
        Me.btn_q_encoder.Size = New System.Drawing.Size(124, 22)
        Me.btn_q_encoder.TabIndex = 22
        Me.btn_q_encoder.TabStop = False
        Me.btn_q_encoder.Text = "Encode Video(s)"
        Me.btn_q_encoder.UseVisualStyleBackColor = False
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(13, 13)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(110, 13)
        Me.Label1.TabIndex = 23
        Me.Label1.Text = "Videos on the Queue:"
        '
        'btn_delete
        '
        Me.btn_delete.BackColor = System.Drawing.SystemColors.Control
        Me.btn_delete.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_delete.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_delete.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_delete.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_delete.Location = New System.Drawing.Point(355, 403)
        Me.btn_delete.Name = "btn_delete"
        Me.btn_delete.Size = New System.Drawing.Size(124, 22)
        Me.btn_delete.TabIndex = 24
        Me.btn_delete.TabStop = False
        Me.btn_delete.Text = "Delete Item"
        Me.btn_delete.UseVisualStyleBackColor = False
        '
        'frmQueue
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(621, 437)
        Me.ControlBox = False
        Me.Controls.Add(Me.btn_delete)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.btn_q_encoder)
        Me.Controls.Add(Me.list_queue)
        Me.Controls.Add(Me.btn_Close)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.MaximizeBox = False
        Me.Name = "frmQueue"
        Me.ShowInTaskbar = False
        Me.Text = " Queue"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents btn_Close As System.Windows.Forms.Button
    Friend WithEvents list_queue As System.Windows.Forms.ListBox
    Friend WithEvents btn_q_encoder As System.Windows.Forms.Button
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents btn_delete As System.Windows.Forms.Button
End Class
