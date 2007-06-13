Imports System.IO

Public Class frmSelect


    Private Sub frmSelect_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Dim break As Boolean = False
        Dim ErrorCounter As Integer = 0

        Try
            Dim file_path As String = Application.StartupPath
            rtf_dvdInfo.Text = ""

            Dim line As StreamReader = File.OpenText(file_path & "\dvdinfo.dat")
            Dim readLine

            readLine = "------- DVD Information -------"
            While readLine <> Nothing

                If (rtf_dvdInfo.Text = "") Then
                    rtf_dvdInfo.Text = readLine
                Else
                    rtf_dvdInfo.Text = rtf_dvdInfo.Text & vbCrLf & readLine
                End If
                readLine = line.ReadLine()

                If break = True Then
                    readLine = Nothing
                ElseIf readLine = "" Then
                    readLine = " "
                    ErrorCounter = ErrorCounter + 1
                    If ErrorCounter = 5 Then
                        readLine = Nothing
                    End If
                    readLine = Nothing
                Else
                    ErrorCounter = 0
                End If

            End While
        Catch ex As Exception
            MessageBox.Show(ex.ToString) ' Debug
        End Try
        '# -- END
    End Sub

    Private Sub btn_close_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_close.Click
        Me.Close()
    End Sub
End Class