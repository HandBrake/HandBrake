Public Class frmSelectDVD

    Private Sub btn_Browse_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_Browse.Click
        Dim filename As String
        If RadioDVD.Checked Then
            DVD_Open.ShowDialog()
            filename = DVD_Open.SelectedPath
            text_source.Text = filename
            frmMain.text_source.Text = filename
            If filename <> "" Then
                frmReadDVD.Show()
                Me.Close()
            End If

        Else
            ISO_Open.ShowDialog()
            filename = ISO_Open.FileName
            text_source.Text = filename
            frmMain.text_source.Text = filename
            If filename <> "" Then
                frmReadDVD.Show()
                Me.Close()
            End If
        End If
    End Sub

    Private Sub btn_close_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_close.Click
        Me.Close()
    End Sub
End Class