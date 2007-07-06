Imports System.IO
Imports System
Imports System.Diagnostics
Imports System.Threading

Public Class frmReadDVD

    '#
    '# Start Reading the DVD as soon as the window launches
    '# Write the output to dvdinfo.dat
    '#
    Private Sub frmStatus_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Dim applicationPath As String = Application.StartupPath

        Try
            Shell("cmd /c """"" + applicationPath + "\hbcli.exe"" -i """ + frmMain.text_source.Text + """" & " -t0 >" + """" + applicationPath + "\dvdinfo.dat""" + " 2>&1""")
        Catch ex As Exception
            MessageBox.Show("Unable to launch the CLI encoder.", "Errir", MessageBoxButtons.OK, MessageBoxIcon.Hand)
            MessageBox.Show(ex.ToString)
        End Try

    End Sub


    Private Sub btn_ok_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_ok.Click
        'Wait til the hbcli process finishes but do this on another thread so the readDVD window doesn't lock up.
        Dim isRunning As Integer
        Dim process2 As Process = New Process
        Dim running As Boolean = True
        Dim hbProcess As Process() = Process.GetProcesses()

        While running
            Thread.Sleep(1000)
            hbProcess = Process.GetProcesses()
            running = False
            Dim processArr2 As Process() = hbProcess
            Dim i As Integer = 0
            While i < CInt(processArr2.Length)
                Dim process1 As Process = processArr2(i)
                If process1.ProcessName.Equals("hbcli") Then
                    running = True
                End If
                i = i + 1
            End While
        End While
        isRunning = 0

        ' Once hbcli.exe has finished. appened --end-- to the end of the file. This will be our EOF marker.
        Dim applicationPath As String = Application.StartupPath
        Dim fileWriter As System.IO.StreamWriter
        fileWriter = File.AppendText(applicationPath & "\dvdinfo.dat")
        fileWriter.WriteLine("--end--")
        fileWriter.Flush()
        fileWriter.Close()

        ' Clean the dvd title dropdown menu so it does not contain any of the previos items
        frmMain.drp_dvdtitle.Items.Clear()


        ' Open dvdinfo.dat for reading and parse its contents. Place parsed contents in dvd.dat
        ' Firstly we need to declare the storage array outside the try/catch statments so it can be used elsewhere.
        Dim dvdInfoArray(150) As String

        Try
            ' File and Line contents varibles
            Dim ReadLine As StreamReader = File.OpenText(applicationPath & "\dvdinfo.dat")
            Dim LineContents As String = ""

            ' DVD info stroage varibles
            Dim titleData As String = ""
            Dim duationData As String = ""
            Dim sizeData As String = ""
            Dim cropdata As String = ""
            Dim chatperData As String = ""
            Dim audioData As String = ""
            Dim subtitleData As String = ""

            Dim fullTitleData As String = ""

            ' Position Pointers
            Dim chapterPointer As Boolean = False
            Dim audioPointer As Boolean = False
            Dim subtitlePointer As Boolean = False
            Dim counter As Integer = 0

            ' Error handling varibles
            Dim titleError As Boolean = False
            Dim readError As Boolean = False

            ' Read every line of the file and place the contents in the approiate varible.
            While Not LineContents.Equals("--end--")
                ' Get all the 1 liner data and set chaper potiner to true when done
                If (LineContents.Contains("exited.")) Then
                    subtitlePointer = False
                    fullTitleData = titleData.Trim + " ~ " + duationData.Trim + " ~ " + sizeData.Trim + " ~ " + cropdata.Trim + " ~ " + chatperData.Trim + " ~ " + audioData.Trim + " ~ " + subtitleData.Trim
                    dvdInfoArray(counter) = fullTitleData
                    add(fullTitleData)
                    counter = counter + 1
                ElseIf (LineContents.Contains("+ title")) Then
                    If (titleData <> Nothing) Then
                        subtitlePointer = False
                        fullTitleData = titleData.Trim + " ~ " + duationData.Trim + " ~ " + sizeData.Trim + " ~ " + cropdata.Trim + " ~ " + chatperData.Trim + " ~ " + audioData.Trim + " ~ " + subtitleData.Trim
                        dvdInfoArray(counter) = fullTitleData
                        add(fullTitleData)
                        counter = counter + 1
                    End If
                    titleData = LineContents
                ElseIf (LineContents.Contains("+ duration")) Then
                    duationData = LineContents
                ElseIf (LineContents.Contains("+ size")) Then
                    sizeData = LineContents
                ElseIf (LineContents.Contains("+ autocrop")) Then
                    cropdata = LineContents
                ElseIf (LineContents.Contains("+ chapters")) Then
                    chatperData = LineContents
                    chapterPointer = True
                End If

                ' Get all the chapter information in 1 varible
                If chapterPointer = True Then
                    If LineContents.Contains("+ audio") Then
                        chapterPointer = False
                        audioPointer = True
                        audioData = LineContents
                    Else
                        If Not chatperData.Equals(LineContents) Then
                            chatperData = chatperData & " & " & LineContents.Trim
                        End If
                    End If
                End If

                ' Get all the audio channel information in 1 varible
                If audioPointer = True Then
                    If LineContents.Contains("+ subtitle") Then
                        audioPointer = False
                        subtitlePointer = True
                        subtitleData = LineContents
                    Else
                        If Not audioData.Equals(LineContents) Then
                            audioData = audioData & " & " & LineContents.Trim
                        End If
                    End If
                End If

                ' Get all the subtitle data into 1 varible
                If subtitlePointer = True Then
                    If LineContents.Contains("+ subtitle") Then
                        subtitleData = LineContents
                    Else
                        If Not subtitleData.Equals(LineContents) Then
                            subtitleData = subtitleData & " & " & LineContents.Trim
                        End If
                    End If
                End If

                ' Handle some of Handbrakes Error outputs if they occur.
                If (LineContents.Contains("No title")) Then
                    titleError = True
                End If

                If (LineContents.Contains("***")) Then
                    readError = True
                End If

                ' Read the next line of the file
                LineContents = ReadLine.ReadLine
            End While

            ' Close the file as it is no longer required
            ReadLine.Close()


            ' Display error messages for errors detected above.
            If readError = True Then
                MessageBox.Show("Some DVD Title information may be missing however you may still be able to select your required title for encoding!", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk)
            End If

            If titleError = True Then
                MessageBox.Show("No Title(s) found. Please make sure you have selected a valid, non-copy protected source.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
            End If

        Catch ex As Exception
            MessageBox.Show("DEBUG: " & ex.ToString)
        End Try



        ' Take the information from dvdInfoArray and output each item in the array to a seperate line in the file.
        Try
            Dim outputWriter As StreamWriter = New StreamWriter(applicationPath & "\dvd.dat")
            Dim dvdCount As Integer = dvdInfoArray.Length
            Dim counter As String = 0

            While counter <> dvdCount
                outputWriter.WriteLine(dvdInfoArray(counter))
                counter = counter + 1
            End While
            outputWriter.Close()

        Catch ex As Exception
            MessageBox.Show("Unable to write Data file. Please make sure the application has admin privileges.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
        End Try

        Me.Close()
    End Sub



    '# A function to Add data to frmSelects Select Title Tab and also populate frmMains dvdTitle Tab
    Function add(ByVal titleData)
        Dim titleInfo() As String
        Dim str(7) As String
        Dim data As String = ""

        titleInfo = titleData.Split("~")
        Try
            str(0) = titleInfo(0).Trim.Substring(8).Replace(":", "") 'Title
            str(1) = titleInfo(1).Trim.Substring(12) ' Duration
        Catch ex As Exception
            MessageBox.Show("Incomplete DVD data found. Please copy the data on the View DVD Information tab and report this error.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
        End Try

        'Now lets add the info to the main form dropdowns
        frmMain.drp_dvdtitle.Items.Add(str(0) & " (" & str(1) & ")")

        Return 0
    End Function

End Class