<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmSelectDVD
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
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(frmSelectDVD))
        Me.RadioDVD = New System.Windows.Forms.RadioButton
        Me.RadioISO = New System.Windows.Forms.RadioButton
        Me.btn_Browse = New System.Windows.Forms.Button
        Me.Label17 = New System.Windows.Forms.Label
        Me.text_source = New System.Windows.Forms.TextBox
        Me.Label1 = New System.Windows.Forms.Label
        Me.btn_close = New System.Windows.Forms.Button
        Me.ISO_Open = New System.Windows.Forms.OpenFileDialog
        Me.DVD_Open = New System.Windows.Forms.FolderBrowserDialog
        Me.SuspendLayout()
        '
        'RadioDVD
        '
        Me.RadioDVD.AutoSize = True
        Me.RadioDVD.Checked = True
        Me.RadioDVD.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.RadioDVD.Location = New System.Drawing.Point(265, 30)
        Me.RadioDVD.Name = "RadioDVD"
        Me.RadioDVD.Size = New System.Drawing.Size(51, 17)
        Me.RadioDVD.TabIndex = 45
        Me.RadioDVD.TabStop = True
        Me.RadioDVD.Text = "DVD"
        Me.RadioDVD.UseVisualStyleBackColor = True
        '
        'RadioISO
        '
        Me.RadioISO.AutoSize = True
        Me.RadioISO.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.RadioISO.Location = New System.Drawing.Point(265, 46)
        Me.RadioISO.Name = "RadioISO"
        Me.RadioISO.Size = New System.Drawing.Size(47, 17)
        Me.RadioISO.TabIndex = 44
        Me.RadioISO.Text = "ISO"
        Me.RadioISO.UseVisualStyleBackColor = True
        '
        'btn_Browse
        '
        Me.btn_Browse.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_Browse.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_Browse.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_Browse.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_Browse.Location = New System.Drawing.Point(321, 35)
        Me.btn_Browse.Name = "btn_Browse"
        Me.btn_Browse.Size = New System.Drawing.Size(78, 22)
        Me.btn_Browse.TabIndex = 41
        Me.btn_Browse.Text = "Browse"
        Me.btn_Browse.UseVisualStyleBackColor = True
        '
        'Label17
        '
        Me.Label17.AutoSize = True
        Me.Label17.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label17.ForeColor = System.Drawing.Color.Black
        Me.Label17.Location = New System.Drawing.Point(-97, 30)
        Me.Label17.Name = "Label17"
        Me.Label17.Size = New System.Drawing.Size(52, 13)
        Me.Label17.TabIndex = 42
        Me.Label17.Text = "Source:"
        '
        'text_source
        '
        Me.text_source.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle
        Me.text_source.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.text_source.Location = New System.Drawing.Point(16, 36)
        Me.text_source.Name = "text_source"
        Me.text_source.Size = New System.Drawing.Size(242, 21)
        Me.text_source.TabIndex = 40
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.Location = New System.Drawing.Point(13, 13)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(126, 13)
        Me.Label1.TabIndex = 46
        Me.Label1.Text = "Select DVD Source"
        '
        'btn_close
        '
        Me.btn_close.FlatAppearance.BorderColor = System.Drawing.Color.Black
        Me.btn_close.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.btn_close.Font = New System.Drawing.Font("Verdana", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btn_close.ForeColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(0, Byte), Integer))
        Me.btn_close.Location = New System.Drawing.Point(321, 77)
        Me.btn_close.Name = "btn_close"
        Me.btn_close.Size = New System.Drawing.Size(78, 22)
        Me.btn_close.TabIndex = 47
        Me.btn_close.Text = "Close"
        Me.btn_close.UseVisualStyleBackColor = True
        '
        'ISO_Open
        '
        Me.ISO_Open.DefaultExt = "iso"
        Me.ISO_Open.Filter = "iso|*.iso"
        '
        'frmReadDVD
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(418, 111)
        Me.Controls.Add(Me.btn_close)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.RadioDVD)
        Me.Controls.Add(Me.RadioISO)
        Me.Controls.Add(Me.btn_Browse)
        Me.Controls.Add(Me.Label17)
        Me.Controls.Add(Me.text_source)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "frmReadDVD"
        Me.Text = "Read DVD"
        Me.TopMost = True
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents RadioDVD As System.Windows.Forms.RadioButton
    Friend WithEvents RadioISO As System.Windows.Forms.RadioButton
    Friend WithEvents btn_Browse As System.Windows.Forms.Button
    Friend WithEvents Label17 As System.Windows.Forms.Label
    Friend WithEvents text_source As System.Windows.Forms.TextBox
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents btn_close As System.Windows.Forms.Button
    Friend WithEvents ISO_Open As System.Windows.Forms.OpenFileDialog
    Friend WithEvents DVD_Open As System.Windows.Forms.FolderBrowserDialog
End Class
