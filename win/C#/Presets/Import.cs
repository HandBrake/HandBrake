using System;
using System.IO;
using System.Xml;
using System.Windows.Forms;
using System.Collections;
using Handbrake.Functions;

namespace Handbrake.Presets
{
    class Import
    {
        public QueryParser importMacPreset(string filename)
        {
            if (!File.Exists(filename))
                return null;

            StreamReader sr = File.OpenText(filename);
            string fromfile = string.Empty;
            int fileChar;
            while ((fileChar = sr.Read()) != -1)
                fromfile += Convert.ToChar(fileChar);

            XmlDocument doc = new XmlDocument();
            doc.LoadXml(fromfile);

            XmlNode root = doc;
            if (!root.HasChildNodes)
            {
                MessageBox.Show(
                    "The Preset file you selected appears to be invlaid or from an older version of HandBrake", "Error",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return null;
            }

            // We'll query a query parser object and use it's public var structures to store all the data.
            // This will allow the preset loader logic to be used instead of writing custom logic just for this file.
            QueryParser queryParsed = new QueryParser();

            /***** Get the Audio Tracks *****/
            XmlNode audioListDict = root.ChildNodes[2].ChildNodes[0].FirstChild.ChildNodes[1];
            ArrayList AudioInfo = new ArrayList();
            for (int i = 0; i < audioListDict.ChildNodes.Count; i++)
            {
                XmlNode audioChannel = audioListDict.ChildNodes[i];
                AudioTrack track = new AudioTrack();

                for (int subi = 0; subi < audioChannel.ChildNodes.Count; subi += 2)
                {
                    // Audio Channel Information is here.
                    string key = audioChannel.ChildNodes[subi].InnerText;
                    string value = audioChannel.ChildNodes[subi + 1].InnerText;
                    switch (key)
                    {
                        case "AudioBitrate":
                            track.Bitrate = value;
                            break;
                        case "AudioEncoder":
                            track.Encoder = value.Replace("AAC (CoreAudio)", "AAC (faac)");
                            break;
                        case "AudioMixdown":
                            track.MixDown = value;
                            break;
                        case "AudioSamplerate":
                            track.SampleRate = value;
                            break;
                        case "AudioTrack":
                            track.Track = value;
                            break;
                        case "AudioTrackDRCSlider":
                            track.DRC = value;
                            break;
                    }
                }
               AudioInfo.Add(track);
            }
            queryParsed.AudioInformation = AudioInfo;

            /***** Get the rest of the settings. *****/
            XmlNode presetSettings = root.ChildNodes[2].ChildNodes[0].FirstChild;
            for (int i = 2; i < presetSettings.ChildNodes.Count; i += 2) // Start from 2 to avoid the audio settings which we don't need.
            {
                string key = presetSettings.ChildNodes[i].InnerText;
                string value = presetSettings.ChildNodes[i + 1].InnerText;

                switch (key)
                {
                    // Output Settings
                    case "FileFormat":
                        queryParsed.Format = value;
                        break;
                    case "Mp4HttpOptimize":
                        queryParsed.OptimizeMP4 = value == "1";
                        break;
                    case "Mp4LargeFile":
                        queryParsed.LargeMP4 = value == "1";
                        break;
                    case "Mp4iPodCompatible":
                        queryParsed.IpodAtom = value == "1";
                        break;

                    // Picture Settings
                    case "PictureAutoCrop":
                        break;
                    case "PictureTopCrop":
                        queryParsed.CropTop = value;
                        break;
                    case "PictureBottomCrop":
                        queryParsed.CropBottom = value;
                        break;
                    case "PictureLeftCrop":
                        queryParsed.CropLeft = value;
                        break;
                    case "PictureRightCrop":
                        queryParsed.CropRight = value;
                        break;
                    case "PictureHeight":
                        queryParsed.Height = int.Parse(value);
                        break;
                    case "PictureWidth":
                        queryParsed.Width = int.Parse(value);
                        break;
                    case "PictureKeepRatio":
                        queryParsed.keepDisplayAsect = value == "1";
                        break;
                    case "PicturePAR":
                        queryParsed.AnamorphicMode = int.Parse(value);
                        break;


                    // Filters
                    case "PictureDeblock":
                        queryParsed.DeBlock = int.Parse(value);
                        break;
                    case "PictureDecomb":
                        queryParsed.Decomb = "Off";
                        if (value == "1") queryParsed.Decomb = "Default";
                        break;
                    case "PictureDecombCustom":
                        if (value != "")
                            queryParsed.Decomb = value;
                        break;
                    case "PictureDecombDeinterlace":
                        // Not Used
                        break;
                    case "PictureDeinterlace":
                        switch (value)
                        {
                            case "0":
                                queryParsed.DeInterlace = "None";
                                break;
                            case "1":
                                queryParsed.DeInterlace = "Fast";
                                break;
                            case "2":
                                queryParsed.DeInterlace = "Slow";
                                break;
                            case "3":
                                queryParsed.DeInterlace = "Slowest";
                                break;
                        }
                        break;
                    case "PictureDeinterlaceCustom":
                        if (value != "")
                            queryParsed.DeInterlace = value;
                        break;
                    case "PictureDenoise":
                        switch (value)
                        {
                            case "0":
                                queryParsed.DeNoise = "None";
                                break;
                            case "1":
                                queryParsed.DeNoise = "Weak";
                                break;
                            case "2":
                                queryParsed.DeNoise = "Medium";
                                break;
                            case "3":
                                queryParsed.DeNoise = "Strong";
                                break;
                        }

                        break;
                    case "PictureDenoiseCustom":
                        if (value != "")
                            queryParsed.DeNoise = value;
                        break;
                    case "PictureDetelecine":
                        queryParsed.DeTelecine = "Off";
                        if (value == "1") queryParsed.DeTelecine = "Default";
                        break;
                    case "PictureDetelecineCustom":
                        if (value != "")
                            queryParsed.DeTelecine = value;
                        break;

                    // Video Tab
                    case "VideoAvgBitrate":
                        queryParsed.Width = int.Parse(value);
                        break;
                    case "VideoEncoder":
                        queryParsed.VideoEncoder = value;
                        break;
                    case "VideoFramerate":
                        queryParsed.VideoFramerate = value;
                        break;
                    case "VideoGrayScale":
                        queryParsed.Grayscale = value == "1";
                        break;
                    case "VideoQualitySlider":
                        queryParsed.VideoQuality = float.Parse(value);
                        break;
                    case "VideoQualityType":
                        break;
                    case "VideoTargetSize":
                        queryParsed.VideoTargetSize = value;
                        break;
                    case "VideoTurboTwoPass":
                        queryParsed.TurboFirstPass = value == "1";
                        break;
                    case "VideoTwoPass":
                        queryParsed.TwoPass = value == "1";
                        break;

                    // Chapter Markers Tab
                    case "ChapterMarkers":
                        queryParsed.ChapterMarkers = value == "1";
                        break;

                    // Advanced x264 tab
                    case "x264Option":
                        queryParsed.H264Query = value;
                        break;

                    // Preset Information
                    case "PresetBuildNumber":
                        queryParsed.PresetBuildNumber = int.Parse(value);
                        break;
                    case "PresetDescription":
                        queryParsed.PresetDescription = value;
                        break;
                    case "PresetName":
                        queryParsed.PresetName = value;
                        break;
                    case "Type":
                        queryParsed.Type = value;
                        break;
                    case "UsesMaxPictureSettings":
                        queryParsed.UsesMaxPictureSettings = value == "1";
                        break;
                    case "UsesPictureFilters":
                        queryParsed.UsesPictureFilters = value == "1";
                        break;
                    case "UsesPictureSettings":
                        queryParsed.UsesPictureSettings = value == "1";
                        break;
                }
            }
            return queryParsed;
        }

        // --- It's the end of the road
    }
}