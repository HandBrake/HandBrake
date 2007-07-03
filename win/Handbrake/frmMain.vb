Imports System.IO
Imports System
Imports System.Diagnostics
Imports System.Threading
Imports System.ComponentModel
Imports System.Windows.Forms


Public Class frmMain


    Private Sub frmMain_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        '# Sets the Version in the bottom corner of the screen.
        Version.Text = System.String.Format(Version.Text, My.Application.Info.Version.Major, My.Application.Info.Version.Minor)

        '# ----------------------------------------------------
        '# Updated check on Startup.
        Dim file_path As String = Application.StartupPath

        Try
            If My.Settings.StartupUpdate = 1 Then
                ' Download the update file
                ' open the file for reading and read the first 2 lines for GUI and CLI versions
                Dim wc As New System.Net.WebClient()
                wc.DownloadFile("http://download.m0k.org/handbrake/windows/update.txt", file_path & "\update.txt")
                wc.Dispose()
                Dim versionStream As StreamReader = File.OpenText(file_path & "\update.txt")
                Dim windowsGUI As String = versionStream.ReadLine()
                Dim windowsCLI As String = versionStream.ReadLine()
                versionStream.Close()

                ' If the version is now the same as the one shown here, Display the update label
                If windowsGUI <> My.Settings.HandbrakeGUIVersion Then
                    lbl_update.Visible = True
                ElseIf windowsCLI <> My.Settings.HandbrakeCLIVersion Then
                    lbl_update.Visible = True
                End If

            End If

        Catch ex As Exception
            '# No need to alert the user if the update fails. Its just annoying.
        End Try

        '#---------------------------------------------------
        '# Load the Last used Settings
        '#---------------------------------------------------
        Try
            If My.Settings.UseUsersDefaultSettings = 1 Then
                'Source
                text_source.Text = My.Settings.DVDSource
                drp_dvdtitle.Text = My.Settings.DVDTitle
                drop_chapterStart.Text = My.Settings.ChapterStart
                drop_chapterFinish.Text = My.Settings.ChapterFinish
                'Destination
                text_destination.Text = My.Settings.VideoDest
                drp_videoEncoder.Text = My.Settings.VideoEncoder
                drp_audioCodec.Text = My.Settings.AudioEncoder
                text_width.Text = My.Settings.Width
                text_height.Text = My.Settings.Height
                'Picture Settings Tab
                drp_crop.Text = My.Settings.CroppingOption
                text_top.Text = My.Settings.CropTop
                text_bottom.Text = My.Settings.CropBottom
                text_left.Text = My.Settings.CropLeft
                text_right.Text = My.Settings.CropRight
                drp_subtitle.Text = My.Settings.Subtitles
                'Video Settings Tab
                text_bitrate.Text = My.Settings.VideoBitrate
                text_filesize.Text = My.Settings.VideoFilesize
                slider_videoQuality.Value = My.Settings.VideoQuality
                check_2PassEncode.CheckState = My.Settings.TwoPass
                check_DeInterlace.CheckState = My.Settings.DeInterlace
                check_grayscale.CheckState = My.Settings.Grayscale
                drp_videoFramerate.Text = My.Settings.Framerate
                CheckPixelRatio.CheckState = My.Settings.PixelRatio
                check_turbo.CheckState = My.Settings.turboFirstPass
                check_largeFile.CheckState = My.Settings.largeFile
                'Audio Settings Tab
                drp_audioBitrate.Text = My.Settings.AudioBitrate
                drp_audioSampleRate.Text = My.Settings.AudioSampleRate
                drp_audioChannels.Text = My.Settings.AudioChannels
                'H264 Tab
                CheckCRF.CheckState = My.Settings.CRF
                rtf_h264advanced.Text = My.Settings.H264
            End If
        Catch ex As Exception
            '# Not actually needed but vb required it to avoid an unhandled exception Interger to String issue. Maybe fix this later
        End Try


        '#---------------------------------------------------
        '# Read DVD at Startup Dialog
        '#---------------------------------------------------
        If My.Settings.ReadDVDatStartup = 1 Then
            frmSelectDVD.Show()
        End If
    End Sub

    '#
    '#
    '# Any Code relating to the frmMain menu bar
    '#
    '#

    Private Sub mnu_exit_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_exit.Click
        Me.Close()
    End Sub

    Private Sub mnu_save_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_save.Click
        'Source
        Dim source As String = text_source.Text
        Dim dvdTitle As String = drp_dvdtitle.Text

        Dim ChapterStart As String = drop_chapterStart.Text
        Dim ChapterFinish As String = drop_chapterFinish.Text
        'Destination
        Dim destination As String = text_destination.Text
        Dim videoEncoder As String = drp_videoEncoder.Text
        Dim audioEncoder As String = drp_audioCodec.Text
        Dim width As String = text_width.Text
        Dim height As String = text_height.Text
        'Picture Settings Tab
        Dim cropTop As String = text_top.Text
        Dim cropBottom As String = text_bottom.Text
        Dim cropLeft As String = text_left.Text
        Dim cropRight As String = text_right.Text
        Dim subtitles As String = drp_subtitle.Text
        'Video Settings Tab
        Dim videoBitrate As String = text_bitrate.Text
        Dim videoFilesize As String = text_filesize.Text
        Dim videoQuality As String = slider_videoQuality.Value
        Dim twoPassEncoding As String = check_2PassEncode.CheckState
        Dim deinterlace As String = check_DeInterlace.CheckState
        Dim grayscale As String = check_grayscale.CheckState
        Dim videoFramerate As String = drp_videoFramerate.Text
        Dim pixelRation As String = CheckPixelRatio.CheckState
        Dim ChapterMarkers As String = Check_ChapterMarkers.CheckState
        Dim turboH264 As String = check_turbo.CheckState
        Dim largeFile As String = check_largeFile.CheckState
        'Audio Settings Tab
        Dim audioBitrate As String = drp_audioBitrate.Text
        Dim audioSampleRate As String = drp_audioSampleRate.Text
        Dim audioChannels As String = drp_audioChannels.Text
        Dim AudioMixDown As String = drp_audioMixDown.Text
        'H264 Tab
        Dim CRF As String = CheckCRF.CheckState
        Dim advH264 As String = rtf_h264advanced.Text

        Dim filename As String
        File_Save.ShowDialog()
        filename = File_Save.FileName
        If (filename <> "") Then
            Try
                Dim ApplicationPath As String = Application.StartupPath
                Dim StreamWriter As StreamWriter = File.CreateText(filename)
                StreamWriter.WriteLine(source)
                StreamWriter.WriteLine(dvdTitle)
                StreamWriter.WriteLine(ChapterStart)
                StreamWriter.WriteLine(ChapterFinish)
                StreamWriter.WriteLine(destination)
                StreamWriter.WriteLine(videoEncoder)
                StreamWriter.WriteLine(audioEncoder)
                StreamWriter.WriteLine(width)
                StreamWriter.WriteLine(height)
                StreamWriter.WriteLine(cropTop)
                StreamWriter.WriteLine(cropBottom)
                StreamWriter.WriteLine(cropLeft)
                StreamWriter.WriteLine(cropRight)
                StreamWriter.WriteLine(subtitles)
                StreamWriter.WriteLine(videoBitrate)
                StreamWriter.WriteLine(videoFilesize)
                StreamWriter.WriteLine(videoQuality)
                StreamWriter.WriteLine(twoPassEncoding)
                StreamWriter.WriteLine(deinterlace)
                StreamWriter.WriteLine(grayscale)
                StreamWriter.WriteLine(videoFramerate)
                StreamWriter.WriteLine(ChapterMarkers)
                StreamWriter.WriteLine(pixelRation)
                StreamWriter.WriteLine(turboH264)
                StreamWriter.WriteLine(largeFile)
                StreamWriter.WriteLine(audioBitrate)
                StreamWriter.WriteLine(audioSampleRate)
                StreamWriter.WriteLine(audioChannels)
                StreamWriter.WriteLine(AudioMixDown)
                StreamWriter.WriteLine(CRF)
                StreamWriter.WriteLine(advH264)
                StreamWriter.Close()
                MessageBox.Show("Your profile has been sucessfully saved.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk)
            Catch
                MessageBox.Show("Unable to write to the file. Please make sure the location has the correct permissions for file writing.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
            End Try
        End If
    End Sub

    Private Sub mnu_open_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_open.Click
        Dim filename As String
        File_Open.ShowDialog()
        filename = File_Open.FileName
        If (filename <> "") Then
            Try
                Dim inputStream As StreamReader = File.OpenText(filename)

                text_source.Text = inputStream.ReadLine()
                drp_dvdtitle.Text = inputStream.ReadLine()
                drop_chapterStart.Text = inputStream.ReadLine()
                drop_chapterFinish.Text = inputStream.ReadLine()
                text_destination.Text = inputStream.ReadLine()
                drp_videoEncoder.Text = inputStream.ReadLine()
                drp_audioCodec.Text = inputStream.ReadLine()
                text_width.Text = inputStream.ReadLine()
                text_height.Text = inputStream.ReadLine()
                text_top.Text = inputStream.ReadLine()
                text_bottom.Text = inputStream.ReadLine()
                text_left.Text = inputStream.ReadLine()
                text_right.Text = inputStream.ReadLine()
                drp_subtitle.Text = inputStream.ReadLine()
                text_bitrate.Text = inputStream.ReadLine()
                text_filesize.Text = inputStream.ReadLine()
                slider_videoQuality.Value = inputStream.ReadLine()
                check_2PassEncode.CheckState = inputStream.ReadLine()
                check_DeInterlace.CheckState = inputStream.ReadLine()
                check_grayscale.CheckState = inputStream.ReadLine()
                drp_videoFramerate.Text = inputStream.ReadLine()
                Check_ChapterMarkers.CheckState = inputStream.ReadLine()
                CheckPixelRatio.CheckState = inputStream.ReadLine()
                check_turbo.CheckState = inputStream.ReadLine()
                check_largeFile.CheckState = inputStream.ReadLine()
                drp_audioBitrate.Text = inputStream.ReadLine()
                drp_audioSampleRate.Text = inputStream.ReadLine()
                drp_audioChannels.Text = inputStream.ReadLine()
                drp_audioMixDown.Text = inputStream.ReadLine()

                'Advanced H264 Options
                CheckCRF.CheckState = inputStream.ReadLine()
                rtf_h264advanced.Text = inputStream.ReadLine()


                ' Fix for SliderValue not appearing when Opening saved file
                SliderValue.Text = slider_videoQuality.Value & "%"

            Catch ex As Exception
                MessageBox.Show("Unable to load profile.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
            End Try
        End If

    End Sub

    Private Sub mnu_about_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_about.Click
        frmAbout.Show()
    End Sub

    Private Sub mnu_update_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_update.Click
        frmUpdate.Show()
    End Sub

    Private Sub mnu_encode_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_encode.Click
        frmQueue.Show()
    End Sub

    Private Sub mnu_ProgramDefaultOptions_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_ProgramDefaultOptions.Click
        'Source
        My.Settings.DVDSource = text_source.Text
        My.Settings.DVDTitle = drp_dvdtitle.Text
        My.Settings.ChapterStart = drop_chapterStart.Text
        My.Settings.ChapterFinish = drop_chapterFinish.Text
        'Destination
        My.Settings.VideoDest = text_destination.Text
        My.Settings.VideoEncoder = drp_videoEncoder.Text
        My.Settings.AudioEncoder = drp_audioCodec.Text
        My.Settings.Width = text_width.Text
        My.Settings.Height = text_height.Text
        'Picture Settings Tab
        My.Settings.CroppingOption = drp_crop.Text
        My.Settings.CropTop = text_top.Text
        My.Settings.CropBottom = text_bottom.Text
        My.Settings.CropLeft = text_left.Text
        My.Settings.CropRight = text_right.Text
        My.Settings.Subtitles = drp_subtitle.Text
        'Video Settings Tab
        My.Settings.VideoBitrate = text_bitrate.Text
        My.Settings.VideoFilesize = text_filesize.Text
        My.Settings.VideoQuality = slider_videoQuality.Value
        My.Settings.TwoPass = check_2PassEncode.CheckState
        My.Settings.DeInterlace = check_DeInterlace.CheckState
        My.Settings.Grayscale = check_grayscale.CheckState
        My.Settings.Framerate = drp_videoFramerate.Text
        My.Settings.PixelRatio = CheckPixelRatio.CheckState
        My.Settings.turboFirstPass = check_turbo.CheckState
        My.Settings.largeFile = check_largeFile.CheckState
        'Audio Settings Tab
        My.Settings.AudioBitrate = drp_audioBitrate.Text
        My.Settings.AudioSampleRate = drp_audioSampleRate.Text
        My.Settings.AudioChannels = drp_audioChannels.Text
        'H264 Tab
        My.Settings.CRF = CheckCRF.CheckState
        My.Settings.H264 = rtf_h264advanced.Text
        My.Settings.Save()
    End Sub

    Private Sub mnu_viewDVDdata_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_viewDVDdata.Click
        frmDvdData.Show()
    End Sub

    'Some Presets
    Private Sub mnu_preset_ipod133_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_preset_ipod133.Click
        CheckPixelRatio.CheckState = CheckState.Unchecked
        text_width.Text = "640"
        text_height.Text = "480"
        drp_videoEncoder.Text = "H.264 (iPod)"
        text_bitrate.Text = "1000"
        text_filesize.Text = ""
        slider_videoQuality.Value = 0
        SliderValue.Text = "0%"
        drp_audioBitrate.Text = "160"
        rtf_h264advanced.Text = ""
        drp_crop.Text = "No Crop"
    End Sub

    Private Sub mnu_preset_ipod178_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_preset_ipod178.Click
        CheckPixelRatio.CheckState = CheckState.Unchecked
        text_width.Text = "640"
        text_height.Text = "352"
        drp_videoEncoder.Text = "H.264 (iPod)"
        text_bitrate.Text = "1000"
        text_filesize.Text = ""
        slider_videoQuality.Value = 0
        SliderValue.Text = "0%"
        drp_audioBitrate.Text = "160"
        rtf_h264advanced.Text = ""
        drp_crop.Text = "No Crop"
    End Sub

    Private Sub mnu_preset_ipod235_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_preset_ipod235.Click
        CheckPixelRatio.CheckState = CheckState.Unchecked
        text_width.Text = "640"
        text_height.Text = "272"
        drp_videoEncoder.Text = "H.264 (iPod)"
        text_bitrate.Text = "1000"
        text_filesize.Text = ""
        slider_videoQuality.Value = 0
        SliderValue.Text = "0%"
        drp_audioBitrate.Text = "160"
        rtf_h264advanced.Text = ""
        drp_crop.Text = "No Crop"
    End Sub

    Private Sub mnu_presetPS3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_presetPS3.Click
        CheckPixelRatio.CheckState = CheckState.Unchecked
        text_width.Text = ""
        text_height.Text = ""
        drp_videoEncoder.Text = "H.264"
        text_bitrate.Text = "3000"
        text_filesize.Text = ""
        slider_videoQuality.Value = 0
        SliderValue.Text = "0%"
        drp_audioBitrate.Text = "160"
        CheckPixelRatio.CheckState = CheckState.Checked
        drp_audioSampleRate.Text = "48"
        rtf_h264advanced.Text = "level=41"
        drp_crop.Text = "No Crop"
    End Sub

    Private Sub mnu_appleTv_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_appleTv.Click
        text_width.Text = ""
        text_height.Text = ""
        drp_videoEncoder.Text = "H.264"
        text_bitrate.Text = "3000"
        text_filesize.Text = ""
        slider_videoQuality.Value = 0
        SliderValue.Text = "0%"
        drp_audioBitrate.Text = "160"
        CheckPixelRatio.CheckState = CheckState.Checked
        drp_audioSampleRate.Text = "48"
        rtf_h264advanced.Text = "bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:no-dct-decimate=1:trellis=2"
        drp_crop.Text = "No Crop"
    End Sub

    Private Sub mnu_options_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_options.Click
        frmOptions.Show()
    End Sub

    Private Sub mnu_wiki_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_wiki.Click
        System.Diagnostics.Process.Start("http://handbrake.m0k.org/trac")
    End Sub

    Private Sub mnu_onlineDocs_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_onlineDocs.Click
        System.Diagnostics.Process.Start("http://handbrake.m0k.org/?page_id=11")
    End Sub

    Private Sub mnu_homepage_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_homepage.Click
        System.Diagnostics.Process.Start("http://handbrake.m0k.org")
    End Sub

    Private Sub mnu_forum_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_forum.Click
        System.Diagnostics.Process.Start("http://handbrake.m0k.org/forum")
    End Sub

    Private Sub mnu_faq_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles mnu_faq.Click
        System.Diagnostics.Process.Start("http://handbrake.m0k.org/trac/wiki/WindowsGuiFaq")
    End Sub

    '#
    '#
    '# Buttons on the frmMain
    '#
    '#

    Private Sub btn_Browse_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_Browse.Click
        Dim filename As String
        text_source.Text = ""
        If RadioDVD.Checked Then
            DVD_Open.ShowDialog()
            filename = DVD_Open.SelectedPath
            text_source.Text = filename
            If filename <> "" Then
                frmReadDVD.Show()
            End If
        Else
            ISO_Open.ShowDialog()
            filename = ISO_Open.FileName
            text_source.Text = filename
            If filename <> "" Then
                frmReadDVD.Show()
            End If
        End If
    End Sub

    Private Sub btn_destBrowse_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_destBrowse.Click
        Dim filename As String
        DVD_Save.ShowDialog()
        filename = DVD_Save.FileName

        If Check_ChapterMarkers.CheckState = 1 Then
            filename = filename.Replace(".mp4", ".m4v").Trim()
        End If

        text_destination.Text = filename.Trim
        Dim DriveLetter() As String = text_destination.Text.Split(":")

        '#
        '# Make sure there is a reasonable amount of space left on the Drive.
        '#
        Try
            Dim FileSys = CreateObject("Scripting.FileSystemObject")
            Dim Drv
            Try
                Drv = FileSys.GetDrive(DriveLetter(0) & ":")

                Dim lAvailableSpace As Long
                lAvailableSpace = Drv.AvailableSpace
                lAvailableSpace = lAvailableSpace / 1024 / 1024 / 1024
                If lAvailableSpace < 4 Then
                    MessageBox.Show("Low on Disk Space. There is: " & lAvailableSpace & "GB Available", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
                End If

                Dim lTotalSpace As Long
                lTotalSpace = Drv.TotalSize
            Finally
                Drv = Nothing
            End Try
        Catch ex As Exception
            ' Ignore the Error - Change this to an IF Statment at some point so it works better.
        End Try

    End Sub

    Private Sub GenerateQuery_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles GenerateQuery.Click
        Dim query As String = GenerateTheQuery()
        Dim ApplicationPath As String = Application.StartupPath
        QueryEditorText.Text = """" + ApplicationPath + "\hbcli.exe""" + query
    End Sub

    Private Sub btn_ClearQuery_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_ClearQuery.Click
        QueryEditorText.Text = ""
    End Sub

    Private Sub btn_h264Clear_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_h264Clear.Click
        rtf_h264advanced.Text = ""
    End Sub

    Private Sub btn_queue_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_queue.Click
        Dim query As String
        query = GenerateTheQuery()
        frmQueue.list_queue.Items.Add(query)
        frmQueue.Show()
    End Sub

    Private Sub btn_encode_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_encode.Click
        Dim query As String
        Dim ApplicationPath As String = Application.StartupPath

        If (frmQueue.list_queue.Items.Count > 0) Then
            MessageBox.Show("You have items on the video queue. If you wish to run the queue please use the encode video button on the queue window." _
                                , "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
        End If

        Try
            If (QueryEditorText.Text = "") Then
                query = GenerateTheQuery()

                Dim params As String = query
                Dim proc As New System.Diagnostics.Process
                proc = System.Diagnostics.Process.Start("""" + ApplicationPath + "\hbcli.exe""", params)

                MessageBox.Show("The Handbrake encoder (CLI) will now start and should be encoding your video.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk)

                'Lets start the process monitor to keep an eye on things.
                hbcliMonitor = New ProcessMonitor()
                Dim t = New Thread(AddressOf hbcliMonitor.tmrProcCheck)
                t.Start()
            Else
                query = QueryEditorText.Text

                Dim params As String = query
                Dim proc As New System.Diagnostics.Process
                proc = System.Diagnostics.Process.Start("""" + ApplicationPath + "\hbcli.exe""", params)
                MessageBox.Show("The Handbrake encoder (CLI) will now start and should be encoding your video.", "Alert", MessageBoxButtons.OK, MessageBoxIcon.Asterisk)

                'Lets start the process monitor to keep an eye on things.
                hbcliMonitor = New ProcessMonitor()
                Dim t = New Thread(AddressOf hbcliMonitor.tmrProcCheck)
                t.Start()
            End If
        Catch ex As Exception
            MessageBox.Show("Unable to launch the HandBrake encoder.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error)
            MessageBox.Show(ex.ToString) ' Debug
        End Try

    End Sub

    Private Sub label_h264_LinkClicked(ByVal sender As System.Object, ByVal e As System.Windows.Forms.LinkLabelLinkClickedEventArgs) Handles label_h264.LinkClicked
        System.Diagnostics.Process.Start("http://handbrake.m0k.org/trac/wiki/x264Options")
    End Sub

    '#
    '#
    '# Dynamic stuff on frm Main
    '#
    '#

    Private Sub drop_chapterFinish_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drop_chapterFinish.SelectedIndexChanged
        QueryEditorText.Text = "" ' Just clearing the quert editor box. Users may forget
        Dim chapterFinish As Integer = drop_chapterFinish.Text
        Dim chapterStart As Integer = drop_chapterStart.Text

        Try
            If (chapterFinish < chapterStart) Then
                MessageBox.Show("Invalid Chapter Range! - Final chapter can not be smaller than the starting chapter.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
            End If
        Catch ex As Exception
            MessageBox.Show("Invalid Character Entered!", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
        End Try
    End Sub

    Private Sub drop_chapterStart_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drop_chapterStart.SelectedIndexChanged
        QueryEditorText.Text = "" ' Just clearing the quert editor box. Users may forget
        Dim chapterFinish As Integer = drop_chapterFinish.Text
        Dim chapterStart As Integer = drop_chapterStart.Text
        Try
            If (chapterStart > chapterFinish) Then
                MessageBox.Show("Invalid Chapter Range! - Start chapter can not be larger than the Final chapter.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
            End If
        Catch ex As Exception
            MessageBox.Show("Invalid Character Entered", "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand)
        End Try
    End Sub

    Private Sub text_bitrate_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles text_bitrate.TextChanged
        text_filesize.Text = ""
        slider_videoQuality.Value = 0
        SliderValue.Text = "0%"
        CheckCRF.CheckState = CheckState.Unchecked
    End Sub

    Private Sub text_filesize_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles text_filesize.TextChanged
        text_bitrate.Text = ""
        slider_videoQuality.Value = 0
        SliderValue.Text = "0%"
        CheckCRF.CheckState = CheckState.Unchecked
    End Sub

    Private Sub slider_videoQuality_Scroll(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles slider_videoQuality.Scroll
        SliderValue.Text = slider_videoQuality.Value.ToString + "%"
        text_bitrate.Text = ""
        text_filesize.Text = ""
    End Sub

    Private Sub text_width_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles text_width.TextChanged
        Try
            If CheckPixelRatio.CheckState = CheckState.Checked Then
                text_width.Text = ""

            Else
                If (text_width.Text Mod 16) <> 0 Then
                    text_width.BackColor = Color.LightCoral
                Else
                    text_width.BackColor = Color.LightGreen
                End If
            End If

            If (Not lbl_Aspect.Text.Equals("Select a Title")) Then
                Dim height As Integer = text_width.Text / lbl_Aspect.Text
                Dim mod16 As Integer = height Mod 16
                height = height - mod16
                text_height.Text = height
            End If
           
        Catch ex As Exception

        End Try
    End Sub

    Private Sub text_height_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles text_height.TextChanged
        Try
            If CheckPixelRatio.CheckState = CheckState.Checked Then
                text_height.Text = ""
            Else
                If (text_height.Text Mod 16) <> 0 Then
                    text_height.BackColor = Color.LightCoral
                Else
                    text_height.BackColor = Color.LightGreen
                End If
            End If
        Catch ex As Exception

        End Try

    End Sub

    Private Sub drp_crop_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drp_crop.SelectedIndexChanged
        If (drp_crop.SelectedItem = "Manual") Then
            text_left.Enabled = True
            text_right.Enabled = True
            text_top.Enabled = True
            text_bottom.Enabled = True
        End If

        If (drp_crop.SelectedItem = "Auto Crop") Then
            text_left.Enabled = False
            text_right.Enabled = False
            text_top.Enabled = False
            text_bottom.Enabled = False
            text_left.Text = ""
            text_right.Text = ""
            text_top.Text = ""
            text_bottom.Text = ""

            If lbl_RecomendedCrop.Text <> "Select a Title" Then
                Dim temp() As String
                temp = lbl_RecomendedCrop.Text.Split("/")
                text_left.Text = temp(2)
                text_right.Text = temp(3)
                text_top.Text = temp(0)
                text_bottom.Text = temp(1)
            End If
        End If

        If (drp_crop.SelectedItem = "No Crop") Then
            text_left.Enabled = False
            text_right.Enabled = False
            text_top.Enabled = False
            text_bottom.Enabled = False
            text_left.Text = "0"
            text_right.Text = "0"
            text_top.Text = "0"
            text_bottom.Text = "0"

        End If
    End Sub

    Private Sub CheckPixelRatio_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CheckPixelRatio.CheckedChanged
        text_width.Text = ""
        text_height.Text = ""
        text_width.BackColor = Color.White
        text_height.BackColor = Color.White
    End Sub

    Private Sub drp_dvdtitle_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drp_dvdtitle.SelectedIndexChanged
        ' If the title changes then the following text values are no longer correct.
        ' Maybe automatically update these in later versions.
        lbl_Aspect.Text = "Select a Title"
        lbl_RecomendedCrop.Text = "Select a Title"
        QueryEditorText.Text = ""

        ' If the title is not automatic then read the dvd.dat file and populate the Subtitles box depending on the title slected.
        If drp_dvdtitle.Text <> "Automatic" Then
            Dim temp() As String
            Dim title As String
            temp = drp_dvdtitle.Text.Split(" ")
            title = temp(0).Trim

            '### Find the line that matches the title number
            Try
                Dim file_path As String = Application.StartupPath
                Dim ReadFile As StreamReader = File.OpenText(file_path & "\dvd.dat")
                Dim ReadLine As String = ""

                ReadLine = ReadFile.ReadLine()
                While ReadLine <> ""
                    Dim TempLine() As String
                    Dim Tempdata() As String
                    Dim TempCount As Integer
                    Dim counter As Integer = 1

                    TempLine = ReadLine.Split("~")
                    If TempLine(0).Replace("+ ", "").Trim.Equals("title " & title & ":") Then
                        '### Set the 2 Title boxes.
                        Tempdata = TempLine(4).Split("&")
                        TempCount = Tempdata.Length

                        Dim chapterNumber() As String
                        Dim chapter As String

                        drop_chapterStart.Items.Clear()
                        drop_chapterFinish.Items.Clear()

                        drop_chapterStart.Text = "1"
                        drop_chapterFinish.Text = TempCount - 1


                        While counter <> TempCount
                            chapterNumber = Tempdata(counter).Split(":")
                            chapter = chapterNumber(0).Replace("+ ", "").Trim

                            drop_chapterStart.Items.Add(chapter)
                            drop_chapterFinish.Items.Add(chapter)

                            counter = counter + 1
                        End While
                        counter = 1 ' Reset the counter for reuse



                        '### Here we populate the subtitle box.
                        Tempdata = TempLine(6).Split("&")
                        TempCount = Tempdata.Length
                        ' Cleanup the previous Subtitle Data
                        drp_subtitle.Items.Clear()
                        drp_subtitle.Items.Add("None")
                        drp_subtitle.Text = "None"

                        While counter <> TempCount
                            drp_subtitle.Items.Add(Tempdata(counter).Trim.Replace("+ ", "").Replace(",", ""))
                            counter = counter + 1
                        End While
                        counter = 1 ' Reset the counter for reuse

                        '### Here we populate the Audio title box
                        Tempdata = TempLine(5).Split("&")
                        TempCount = Tempdata.Length
                        ' Cleanup the previous Subtitle Data
                        drp_audioChannels.Items.Clear()
                        drp_audioChannels.Items.Add("Automatic")
                        drp_audioChannels.Text = "Automatic"

                        While counter <> TempCount
                            Dim temporyvalues() As String = Tempdata(counter).Trim.Replace("+ ", "").Replace(",", "").Split(" ")
                            drp_audioChannels.Items.Add(temporyvalues(0) & " " & temporyvalues(1) & " " & temporyvalues(2) & " " & temporyvalues(3) & ")")
                            counter = counter + 1
                        End While
                        counter = 1 ' Reset the counter for reuse

                        '### Here we Set the Aspect Ratio text
                        Tempdata = TempLine(2).Split(",")
                        lbl_Aspect.Text = Tempdata(1).Replace(",", "").Replace("aspect: ", "").Trim

                        '### Finally Set the Recommended Crop Text
                        Tempdata = TempLine(3).Split(" ")
                        lbl_RecomendedCrop.Text = Tempdata(3).Replace(",", "").Trim

                        '# Stop the while loop
                        ReadLine = ""

                    Else
                        ReadLine = ReadFile.ReadLine()
                    End If
                End While
                ReadFile.Close()
            Catch ex As Exception
                ' No need to display an error, The dropdowns will simply not update if a problem occurs here.
            End Try
        Else
            ' If Automatic is selected or the user types in the box, then Clear the Subtitle and Audio Dropdowns
            drp_audioChannels.Items.Clear()
            drp_audioChannels.Items.Add("Automatic")
            drp_audioChannels.Text = "Automatic"
            drp_subtitle.Items.Clear()
            drp_subtitle.Items.Add("None")
            drp_subtitle.Text = "None"
        End If
    End Sub

    Private Sub drp_audioCodec_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drp_audioCodec.SelectedIndexChanged
        'Dim Channels As String = drp_audioChannels.Text

        ' If Channels = "Automatic" Then
        'Channels = "2.0"
        'Else
        'Dim ChanData() As String = Channels.Trim.Split(" ")
        'MessageBox.Show(ChanData.Length)
        'If ChanData.Length <> 0 Then
        'Channels = ChanData(2).Replace("(", "").Replace(")", "")
        'MessageBox.Show(Channels)
        'End If
        'End If
        drp_audioMixDown.Items.Clear()
        If drp_audioCodec.Text = "AAC" Then
            drp_audioMixDown.Items.Add("Mono")
            drp_audioMixDown.Items.Add("Stereo")
            drp_audioMixDown.Items.Add("Dolby Surround")
            drp_audioMixDown.Items.Add("Dolby Pro Logic II")
            drp_audioMixDown.Items.Add("6 Channel Discrete")
            '# Need to impliment
            '# 5.1 will will show 6ch dpl2
            '# 5.0 will show dpl2 but not 6ch
            '# Everything else, mono, stero, dpl1
            drp_audioBitrate.Items.Clear()
            drp_audioBitrate.Items.Add("32")
            drp_audioBitrate.Items.Add("40")
            drp_audioBitrate.Items.Add("48")
            drp_audioBitrate.Items.Add("56")
            drp_audioBitrate.Items.Add("64")
            drp_audioBitrate.Items.Add("80")
            drp_audioBitrate.Items.Add("86")
            drp_audioBitrate.Items.Add("112")
            drp_audioBitrate.Items.Add("128")
            drp_audioBitrate.Items.Add("160")

        Else
            drp_audioMixDown.Items.Add("Stereo")
            drp_audioMixDown.Items.Add("Dolby Surround")
            drp_audioMixDown.Items.Add("Dolby Pro Logic II")

            drp_audioBitrate.Items.Clear()
            drp_audioBitrate.Items.Add("32")
            drp_audioBitrate.Items.Add("40")
            drp_audioBitrate.Items.Add("48")
            drp_audioBitrate.Items.Add("56")
            drp_audioBitrate.Items.Add("64")
            drp_audioBitrate.Items.Add("80")
            drp_audioBitrate.Items.Add("86")
            drp_audioBitrate.Items.Add("112")
            drp_audioBitrate.Items.Add("128")
            drp_audioBitrate.Items.Add("160")
            drp_audioBitrate.Items.Add("192")
            drp_audioBitrate.Items.Add("224")
            drp_audioBitrate.Items.Add("256")
            drp_audioBitrate.Items.Add("320")
            drp_audioBitrate.Items.Add("384")

        End If
        'mono
        'stereo
        'dpl1
        'dpl2
        '6ch

    End Sub

    Private Sub drp_audioMixDown_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drp_audioMixDown.SelectedIndexChanged
        If drp_audioCodec.Text = "AAC" Then
            If drp_audioMixDown.Text = "6 Channel Discrete" Then
                drp_audioBitrate.Items.Clear()
                drp_audioBitrate.Items.Add("32")
                drp_audioBitrate.Items.Add("40")
                drp_audioBitrate.Items.Add("48")
                drp_audioBitrate.Items.Add("56")
                drp_audioBitrate.Items.Add("64")
                drp_audioBitrate.Items.Add("80")
                drp_audioBitrate.Items.Add("86")
                drp_audioBitrate.Items.Add("112")
                drp_audioBitrate.Items.Add("128")
                drp_audioBitrate.Items.Add("160")
                drp_audioBitrate.Items.Add("192")
                drp_audioBitrate.Items.Add("224")
                drp_audioBitrate.Items.Add("256")
                drp_audioBitrate.Items.Add("320")
                drp_audioBitrate.Items.Add("384")
            End If
        End If
    End Sub

    Private Sub Check_ChapterMarkers_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Check_ChapterMarkers.CheckedChanged
        Dim destination As String = text_destination.Text
        If text_destination.Text.Contains(" ") Then

        End If
        destination = destination.Replace(".mp4", ".m4v")
        text_destination.Text = destination
    End Sub

    Private Sub check_largeFile_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles check_largeFile.Click
        If (Not text_destination.Text.Contains(".mp4")) Then
            MessageBox.Show("This option is only compatible with the mp4 file container.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
            check_largeFile.CheckState = CheckState.Unchecked
        End If
    End Sub

    Private Sub check_turbo_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles check_turbo.Click
        If (Not drp_videoEncoder.Text.Contains("H.264")) Then
            MessageBox.Show("This option is only compatible with the H.264 encoder's", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
            check_turbo.CheckState = CheckState.Unchecked
        End If
    End Sub

    Private Sub drp_videoEncoder_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles drp_videoEncoder.SelectedIndexChanged
        ' Turn off some options which are H.264 only when the user selects a non h.264 encoder
        If (Not drp_videoEncoder.Text.Contains("H.264")) Then
            check_turbo.CheckState = CheckState.Unchecked
            CheckCRF.CheckState = CheckState.Unchecked
        End If
    End Sub

    Private Sub CheckCRF_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CheckCRF.Click
        If (slider_videoQuality.Value = 0) Then
            MessageBox.Show("This option is can only be used with the 'Video Quality' slider.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
            CheckCRF.CheckState = CheckState.Unchecked
        End If
    End Sub

    '#
    '#
    '# Functions
    '#
    '#

    Function GenerateTheQuery()

        'Source
        Dim source As String = text_source.Text
        Dim dvdTitle As String = drp_dvdtitle.Text
        Dim chapterStart As String = drop_chapterStart.Text
        Dim chapterFinish As String = drop_chapterFinish.Text
        Dim totalChapters As String = drop_chapterFinish.Items.Count - 1
        Dim dvdChapter As String = ""



        If (source = "") Then
            MessageBox.Show("No Source has been selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
        Else
            source = " -i " + """" + source + """"
        End If

        If (dvdTitle = "Automatic") Then
            dvdTitle = ""
        Else
            Dim titleInfo() As String
            titleInfo = dvdTitle.Split(" ")
            dvdTitle = " -t " + titleInfo(0)
        End If

        If (chapterFinish.Equals("Auto") And chapterStart.Equals("Auto")) Then
            dvdChapter = ""
        ElseIf (chapterFinish = totalChapters & chapterStart > 1) Then
            dvdChapter = ""
        ElseIf chapterFinish = chapterStart Then
            dvdChapter = " -c " + chapterStart
        Else
            dvdChapter = " -c " + chapterStart + "-" + chapterFinish
        End If

        Dim querySource As String = source + dvdTitle + dvdChapter
        '----------------------------------------------------------------------

        'Destination
        Dim destination As String = text_destination.Text
        Dim videoEncoder As String = drp_videoEncoder.Text
        Dim audioEncoder As String = drp_audioCodec.Text
        Dim width As String = text_width.Text
        Dim height As String = text_height.Text

        If (destination = "") Then
            MessageBox.Show("No destination has been selected.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning)
        Else
            destination = " -o " + """" + destination + """"
        End If

        If (videoEncoder = "Mpeg 4") Then
            videoEncoder = " -e ffmpeg"
        ElseIf (videoEncoder = "Xvid") Then
            videoEncoder = " -e xvid"
        ElseIf (videoEncoder = "H.264") Then
            videoEncoder = " -e x264"
        ElseIf (videoEncoder = "H.264 Baseline 1.3") Then
            videoEncoder = " -e x264b13"
        ElseIf (videoEncoder = "H.264 (iPod)") Then
            videoEncoder = " -e x264b30"
        End If

        If (audioEncoder = "AAC") Then
            audioEncoder = " -E faac"
        ElseIf (audioEncoder = "MP3") Then
            audioEncoder = " -E lame"
        ElseIf (audioEncoder = "Vorbis") Then
            audioEncoder = " -E vorbis"
        ElseIf (audioEncoder = "AC3") Then
            audioEncoder = " -E ac3"
        End If

        If (width <> "") Then
            width = " -w " + width
        End If

        If (height <> "") Then
            height = " -l " + height
        End If

        Dim queryDestination As String = destination + videoEncoder + audioEncoder + width + height
        '----------------------------------------------------------------------

        'Picture Settings Tab
        Dim cropSetting As String = drp_crop.Text
        Dim cropTop As String = text_top.Text
        Dim cropBottom As String = text_bottom.Text
        Dim cropLeft As String = text_left.Text
        Dim cropRight As String = text_right.Text
        Dim subtitles As String = drp_subtitle.Text
        Dim cropOut As String = "" 'Returns Crop Query

        If cropSetting = "Auto Crop" Then
            cropOut = ""
        ElseIf cropSetting = "No Crop" Then
            cropOut = " --crop 0:0:0:0 "
        Else
            cropOut = " --crop " + cropTop + ":" + cropBottom + ":" + cropLeft + ":" + cropRight
        End If

        If (subtitles = "None") Then
            subtitles = ""
        ElseIf (subtitles = "") Then
            subtitles = ""
        Else
            Dim tempSub() As String
            tempSub = subtitles.Split(" ")
            subtitles = " -s " + tempSub(0)
        End If

        Dim queryPictureSettings As String = cropOut + subtitles
        '----------------------------------------------------------------------

        'Video Settings Tab
        Dim videoBitrate As String = text_bitrate.Text
        Dim videoFilesize As String = text_filesize.Text
        Dim videoQuality As String = slider_videoQuality.Value
        Dim twoPassEncoding As String = check_2PassEncode.CheckState
        Dim deinterlace As String = check_DeInterlace.CheckState
        Dim grayscale As String = check_grayscale.CheckState
        Dim videoFramerate As String = drp_videoFramerate.Text
        Dim pixelRatio As String = CheckPixelRatio.CheckState
        Dim ChapterMarkers As String = Check_ChapterMarkers.CheckState
        Dim turboH264 As String = check_turbo.CheckState
        Dim largeFile As String = check_largeFile.CheckState

        If (videoBitrate <> "") Then
            videoBitrate = " -b " + videoBitrate
        End If

        If (videoFilesize <> "") Then
            videoFilesize = " -S " + videoFilesize
        End If

        'Video Quality Setting
        If (videoQuality = "0") Then
            videoQuality = ""
        Else
            videoQuality = videoQuality / 100
            If videoQuality = 1 Then
                videoQuality = "1.0"
            End If
            videoQuality = " -q " + videoQuality
        End If

        If (twoPassEncoding = 1) Then
            twoPassEncoding = " -2 "
        Else
            twoPassEncoding = ""
        End If

        If (deinterlace = 1) Then
            deinterlace = " -d "
        Else
            deinterlace = ""
        End If

        If (grayscale = 1) Then
            grayscale = " -g "
        Else
            grayscale = ""
        End If

        If (videoFramerate = "Automatic") Then
            videoFramerate = ""
        Else
            videoFramerate = " -r " + videoFramerate
        End If

        If (pixelRatio = 1) Then
            pixelRatio = " -p "
        Else
            pixelRatio = ""
        End If

        If (ChapterMarkers = 1) Then
            ChapterMarkers = " -m "
        Else
            ChapterMarkers = ""
        End If

        If (turboH264 = 1) Then
            turboH264 = " -T "
        Else
            turboH264 = ""
        End If

        If (largeFile = 1) Then
            largeFile = " -4 "
        Else
            largeFile = ""
        End If


        Dim queryVideoSettings As String = _
        videoBitrate + videoFilesize + videoQuality + twoPassEncoding + deinterlace + grayscale + videoFramerate + pixelRatio + ChapterMarkers + turboH264 + largeFile
        '----------------------------------------------------------------------

        'Audio Settings Tab
        Dim audioBitrate As String = drp_audioBitrate.Text
        Dim audioSampleRate As String = drp_audioSampleRate.Text
        Dim audioChannels As String = drp_audioChannels.Text
        Dim Mixdown As String = drp_audioMixDown.Text
        Dim SixChannelAudio As String = ""

        If (audioBitrate <> "") Then
            audioBitrate = " -B " + audioBitrate
        End If

        If (audioSampleRate <> "") Then
            audioSampleRate = " -R " + audioSampleRate
        End If


        If (audioChannels = "Automatic") Then
            audioChannels = ""
        ElseIf (audioChannels = "") Then
            audioChannels = ""
        Else
            Dim tempSub() As String
            tempSub = audioChannels.Split(" ")
            audioChannels = " -a " + tempSub(0)
        End If


        If (Mixdown = "Automatic") Then
            Mixdown = ""
        ElseIf Mixdown = "Mono" Then
            Mixdown = "mono"
        ElseIf Mixdown = "Stereo" Then
            Mixdown = "stereo"
        ElseIf Mixdown = "Dolby Surround" Then
            Mixdown = "dpl1"
        ElseIf Mixdown = "Dolby Pro Logic II" Then
            Mixdown = "dpl2"
        ElseIf Mixdown = "6 Channel Discrete" Then
            Mixdown = "6ch"
        Else
            Mixdown = "stero"
        End If

        If (Mixdown <> "") Then
            SixChannelAudio = " -6 " & Mixdown
        Else
            SixChannelAudio = ""
        End If



        Dim queryAudioSettings As String = audioBitrate + audioSampleRate + audioChannels + SixChannelAudio
        '----------------------------------------------------------------------


        ' H.264 Tab
        Dim CRF As String = CheckCRF.CheckState
        Dim h264Advanced = rtf_h264advanced.Text

        If (CRF = 1) Then
            CRF = " -Q "
        Else
            CRF = ""
        End If

        If (h264Advanced = "") Then
            h264Advanced = ""
        Else
            h264Advanced = " -x " + h264Advanced
        End If

        Dim h264Settings As String = CRF + h264Advanced
        '----------------------------------------------------------------------

        'Processors (Program Settings)
        Dim processors As String = My.Settings.Processors

        ' Number of Processors Handler
        If (processors = "Automatic") Then
            processors = ""
        Else
            processors = " -C " + processors + " "
        End If

        Dim queryAdvancedSettings As String = processors
        '----------------------------------------------------------------------

        ' Verbose option (Program Settings)
        Dim verbose As String = ""
        If My.Settings.verbose = 1 Then
            verbose = " -v "
        End If
        '----------------------------------------------------------------------


        Return querySource + queryDestination + queryPictureSettings + queryVideoSettings + h264Settings + queryAudioSettings + queryAdvancedSettings + verbose

    End Function


    '#
    '#
    '# hbcli.exe Handling. Some clever multi-threaded code to monitor the encode process.
    '#
    '#
    ' Stage 1
    ' Lets watch the hbcli process and when it finishes then Raise Event ThreadComplete
    Dim WithEvents hbcliMonitor As ProcessMonitor
    Public Class ProcessMonitor
        Public isRunning As Integer = 1

        Public Event ThreadComplete(ByVal isRunning As Integer)

        Public Sub tmrProcCheck()
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
            RaiseEvent ThreadComplete(isRunning)
        End Sub
    End Class
    ' Stage 2
    ' The hbcli processes has exited at this point. Lets throw a messagebox at the user telling him the enocode has completed.
    Sub TheadCompletedMonitor(ByVal isRunning As Integer) Handles hbcliMonitor.ThreadComplete
        Dim ApplicationPath As String = Application.StartupPath ' The applications start parth
        MessageBox.Show("The encoding process has ended.", "Status", MessageBoxButtons.OK, MessageBoxIcon.Asterisk)

    End Sub
    '-----------------------------------------------

End Class