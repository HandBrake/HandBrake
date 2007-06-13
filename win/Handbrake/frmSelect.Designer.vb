<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmSelect
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
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmSelect))
        Me.Label2 = New System.Windows.Forms.Label
        Me.rtf_dvdInfo = New System.Windows.Forms.RichTextBox
        Me.btn_close = New System.Windows.Forms.Button
        Me.SuspendLayout()
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(12, 9)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(518, 26)
        Me.Label2.TabIndex = 27
        Me.Label2.Text = "Handbrake's DVD information output in an unparsed form." & Global.Microsoft.VisualBasic.ChrW(13) & Global.Microsoft.VisualBasic.ChrW(10) & "Note if you have not sca" & _
            "nned the DVD this feature will display the information for the last DVD that was" & _
            " read."
        '
        'rtf_dvdInfo
        '
        Me.rtf_dvdInfo.BorderStyle = System.Windows.Forms.BorderStyle.None
        Me.rtf_dvdInfo.Location = New System.Drawing.Point(15, 52)
        Me.rtf_dvdInfo.Name = "rtf_dvdInfo"
        Me.rtf_dvdInfo.Size = New System.Drawing.Size(515, 403)
        Me.rtf_dvdInfo.TabIndex = 26
        Me.rtf_dvdInfo.Text = ""
        '
        'btn_close
        '
        Me.btn_close.BackColor = System.Drawing.SystemColors.ActiveBorder
        Me.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_close.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_close.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_close.Location = New System.Drawing.Point(420, 463)
        Me.btn_close.Name = "btn_close"
        Me.btn_close.Size = New System.Drawing.Size(110, 22)
        Me.btn_close.TabIndex = 20
        Me.btn_close.TabStop = False
        Me.btn_close.Text = "Close Window"
        Me.btn_close.UseVisualStyleBackColor = False
        '
        'frmSelect
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(545, 493)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.rtf_dvdInfo)
        Me.Controls.Add(Me.btn_close)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "frmSelect"
        Me.Text = "Read DVD"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents btn_close As System.Windows.Forms.Button
    Friend WithEvents rtf_dvdInfo As System.Windows.Forms.RichTextBox
    Friend WithEvents Label2 As System.Windows.Forms.Label
End Class
