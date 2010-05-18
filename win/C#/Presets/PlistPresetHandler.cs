/*  PlistPresetHandler.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Presets
{
    using System;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Windows.Forms;
    using System.Xml;
    using Functions;
    using Model;

    /// <summary>
    /// Plist Preset Converter
    /// </summary>
    public class PlistPresetHandler
    {
        #region Import

        public static QueryParser Import(string filename)
        {
            XmlNode root = loadFile(filename);
            if (root == null) return null;
            
            // We'll query a query parser object and use it's public var structures to store all the data.
            // This will allow the preset loader logic to be used instead of writing custom logic just for this file.
            QueryParser queryParsed = new QueryParser();
            string qualityMode = string.Empty;
          

            #region Get a List of Audio Track Objects
            XmlNode audioListDict = root.ChildNodes[2].ChildNodes[0].FirstChild.ChildNodes[1];
            ArrayList audioTracks = new ArrayList();

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
                audioTracks.Add(track);
                queryParsed.AudioInformation = audioTracks;
            }
            #endregion

            #region Parse the reset of the plist keys into local variables

            XmlNode presetSettings = root.ChildNodes[2].ChildNodes[0].FirstChild;

            // Start from 2 to avoid the audio settings which we don't need.
            for (int i = 2; i < presetSettings.ChildNodes.Count; i += 2)
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
                        // Not used
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
                        queryParsed.KeepDisplayAsect = value == "1";
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
                        // Don't place custom here as it's handled in the filter panel
                        if (value == "2") queryParsed.Decomb = "Default";
                        break;
                    case "PictureDecombCustom":
                        if (value != string.Empty)
                            queryParsed.Decomb = value;
                        break;
                    case "PictureDecombDeinterlace":
                        // Not Used
                        break;
                    case "PictureDeinterlace":
                        switch (value)
                        {
                            case "0":
                                queryParsed.DeInterlace = "Off";
                                break;
                            // Don't place custom here as it's handled in the filter panel
                            case "2":
                                queryParsed.DeInterlace = "Fast";
                                break;
                            case "3":
                                queryParsed.DeInterlace = "Slow";
                                break;
                            case "4":
                                queryParsed.DeInterlace = "Slowest";
                                break;
                        }
                        break;
                    case "PictureDeinterlaceCustom":
                        if (value != string.Empty)
                            queryParsed.DeInterlace = value;
                        break;
                    case "PictureDenoise":
                        switch (value)
                        {
                            case "0":
                                queryParsed.DeNoise = "Off";
                                break;
                            // Don't place custom here as it's handled in the filter panel
                            case "2":
                                queryParsed.DeNoise = "Weak";
                                break;
                            case "3":
                                queryParsed.DeNoise = "Medium";
                                break;
                            case "4":
                                queryParsed.DeNoise = "Strong";
                                break;
                        }

                        break;
                    case "PictureDenoiseCustom":
                        if (value != string.Empty)
                            queryParsed.DeNoise = value;
                        break;
                    case "PictureDetelecine":
                        queryParsed.DeTelecine = "Off";
                        if (value == "1") queryParsed.DeTelecine = "Default";
                        break;
                    case "PictureDetelecineCustom":
                        if (value != string.Empty)
                            queryParsed.DeTelecine = value;
                        break;

                    // Video Tab
                    case "VideoAvgBitrate":
                        queryParsed.AverageVideoBitrate = value;
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
                    case "VideoQualityType": // The Type of Quality Mode used
                        qualityMode = value;
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

            // Kill any Quality values we don't need.
            switch (qualityMode)
            {
                case "0": // FileSize
                    queryParsed.VideoQuality = -1;
                    queryParsed.AverageVideoBitrate = null;
                    break;
                case "1": // Avg Bitrate
                    queryParsed.VideoQuality = -1;
                    queryParsed.VideoTargetSize = null;
                    break;
                case "2": // CQ
                    queryParsed.AverageVideoBitrate = null;
                    queryParsed.VideoTargetSize = null;
                    break;
            }
            #endregion

            return queryParsed;
        }

        private static XmlNode loadFile(string filename)
        {
            try
            {
                XmlNode root;

                if (!File.Exists(filename))
                    return null;

                StreamReader sr = File.OpenText(filename);
                string fromfile = string.Empty;
                int fileChar;
                while ((fileChar = sr.Read()) != -1)
                    fromfile += Convert.ToChar(fileChar);

                XmlDocument doc = new XmlDocument();
                doc.LoadXml(fromfile);

                root = doc;
                if (!root.HasChildNodes)
                {
                    MessageBox.Show(
                        "The Preset file you selected appears to be invlaid or from an older version of HandBrake",
                        "Error",
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return null;
                }

                return root;
            }
            catch (Exception)
            {
                MessageBox.Show(
                    "The Preset file you selected appears to be invlaid or from an older version of HandBrake.\n\n Please note, if you are exporting from the MacGui you may need to rebuild your preset so that it uses the current preset plist format.\n The MacGui does not currently update user presets automatically.",
                    "Error",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return null;
            } 
        }
        #endregion

        #region Export

        public static void Export(string path, QueryParser parsed)
        {
            XmlTextWriter xmlWriter;

            xmlWriter = new XmlTextWriter(path, Encoding.UTF8);

            // Header
            xmlWriter.WriteStartDocument();
            xmlWriter.WriteDocType("plist", "-//Apple//DTD PLIST 1.0//EN",
                                @"http://www.apple.com/DTDs/PropertyList-1.0.dtd", null);

            xmlWriter.WriteStartElement("plist");
            xmlWriter.WriteStartElement("array");

            // Add New Preset Here. Can write multiple presets here if required in future.
            WritePreset(xmlWriter, parsed);

            // Footer
            xmlWriter.WriteEndElement();
            xmlWriter.WriteEndElement();

            xmlWriter.WriteEndDocument();

            // Closeout
            xmlWriter.Close();
        }

        private static void WritePreset(XmlTextWriter xmlWriter, QueryParser parsed)
        {
            xmlWriter.WriteStartElement("dict");
            AudioListArrayDict(xmlWriter, parsed);
            AddEncodeSettings(xmlWriter, parsed);

            xmlWriter.WriteEndElement();
        }

        private static void AddEncodeSettings(XmlTextWriter xmlWriter, QueryParser parsed)
        {
            AddEncodeElement(xmlWriter, "ChapterMarkers", "integer", parsed.ChapterMarkers ? "1" : "0");
            AddEncodeElement(xmlWriter, "Default", "integer", string.Empty); // TODO
            AddEncodeElement(xmlWriter, "FileFormat", "string", parsed.Format);
            AddBooleanElement(xmlWriter, "Folder", true); // TODO 
            AddEncodeElement(xmlWriter, "Mp4HttpOptimize", "integer", parsed.OptimizeMP4 ? "1" : "0");
            AddEncodeElement(xmlWriter, "Mp4LargeFile", "integer", parsed.LargeMP4 ? "1" : "0");
            AddEncodeElement(xmlWriter, "Mp4iPodCompatible", "integer", parsed.IpodAtom ? "1" : "0");
            AddEncodeElement(xmlWriter, "PictureAutoCrop", "integer", string.Empty); // TODO
            AddEncodeElement(xmlWriter, "PictureBottomCrop", "integer", parsed.CropBottom);

            // Filters - TODO Requires Some Logic to convert modes
            AddEncodeElement(xmlWriter, "PictureDeblock", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDecomb", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDecombDeinterlace", "integer", string.Empty); // TODO Requires Logic
            AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDenoise", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDetelecine", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "PictureDetelecineCustom", "string", string.Empty);

            AddEncodeElement(xmlWriter, "PictureHeight", "integer", parsed.Height.ToString());
            AddEncodeElement(xmlWriter, "PictureKeepRatio", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "PictureLeftCrop", "integer", parsed.CropLeft);
            AddEncodeElement(xmlWriter, "PicturePAR", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "PictureRightCrop", "integer", parsed.CropRight);
            AddEncodeElement(xmlWriter, "PictureTopCrop", "integer", parsed.CropTop);
            AddEncodeElement(xmlWriter, "PictureWidth", "integer", parsed.Width.ToString());

            AddEncodeElement(xmlWriter, "PresetBuildNumber", "string", string.Empty);
            AddEncodeElement(xmlWriter, "PresetDescription", "string", string.Empty);
            AddEncodeElement(xmlWriter, "PresetName", "string", string.Empty);
            AddEncodeElement(xmlWriter, "Type", "integer", string.Empty);

            AddEncodeElement(xmlWriter, "UsesMaxPictureSettings", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "UsesPictureFilters", "integer", string.Empty);
            AddEncodeElement(xmlWriter, "UsesPictureSettings", "integer", string.Empty);

            AddEncodeElement(xmlWriter, "VideoAvgBitrate", "string", string.Empty);
            AddEncodeElement(xmlWriter, "VideoEncoder", "string", string.Empty);
            AddEncodeElement(xmlWriter, "VideoFramerate", "string", string.Empty);

            AddEncodeElement(xmlWriter, "VideoGrayScale", "integer", parsed.Grayscale ? "1" : "0");
            AddEncodeElement(xmlWriter, "VideoQualitySlider", "real", parsed.VideoQuality.ToString());
            AddEncodeElement(xmlWriter, "VideoQualityType", "integer", string.Empty); // TODO Requires Logic
            AddEncodeElement(xmlWriter, "VideoTargetSize", "string", parsed.VideoTargetSize);
            AddEncodeElement(xmlWriter, "VideoTurboTwoPass", "integer", parsed.TurboFirstPass ? "1" : "0");
            AddEncodeElement(xmlWriter, "VideoTwoPass", "integer", parsed.TwoPass ? "1" : "0");

            AddEncodeElement(xmlWriter, "x264Option", "string", parsed.H264Query);
        }

        private static void AddBooleanElement(XmlTextWriter xmlWriter, string keyName, bool value)
        {
            xmlWriter.WriteStartElement("key");
            xmlWriter.WriteString(keyName);
            xmlWriter.WriteEndElement();
            if (value)
            {
                xmlWriter.WriteStartElement("true");
                xmlWriter.WriteEndElement();
            }
            else
            {
                xmlWriter.WriteStartElement("false");
                xmlWriter.WriteEndElement();
            }
        }

        private static void AddEncodeElement(XmlTextWriter xmlWriter, string keyName, string type, string value)
        {
            xmlWriter.WriteElementString("key", keyName);
            xmlWriter.WriteElementString(type, value);
        }

        private static void AudioListArrayDict(XmlTextWriter xmlWriter, QueryParser parsed)
        {
            xmlWriter.WriteStartElement("key");
            xmlWriter.WriteString("AudioList");
            xmlWriter.WriteEndElement();

            xmlWriter.WriteStartElement("array");
            foreach (AudioTrack track in parsed.AudioInformation)
            {
                AddAudioItem(xmlWriter, track);
            }
            xmlWriter.WriteEndElement();
        }

        private static void AddAudioItem(XmlTextWriter xmlWriter, AudioTrack audioTrack)
        {
            string bitrate = audioTrack.Bitrate;
            string encoder = audioTrack.Encoder;
            string mixdown = audioTrack.MixDown;
            string sample = audioTrack.SampleRate;
            int track;
            int.TryParse(audioTrack.Track, out track);
            string drc = audioTrack.DRC;
            string desc = "Automatic";

            xmlWriter.WriteStartElement("dict");

            xmlWriter.WriteElementString("key", "AudioBitrate");
            xmlWriter.WriteElementString("string", bitrate);

            xmlWriter.WriteElementString("key", "AudioEncoder");
            xmlWriter.WriteElementString("string", encoder);

            xmlWriter.WriteElementString("key", "AudioMixdown");
            xmlWriter.WriteElementString("string", mixdown);

            xmlWriter.WriteElementString("key", "AudioSamplerate");
            xmlWriter.WriteElementString("string", sample);

            xmlWriter.WriteElementString("key", "AudioTrack");
            xmlWriter.WriteElementString("integer", track.ToString());

            xmlWriter.WriteElementString("key", "AudioTrackDRCSlider");
            xmlWriter.WriteElementString("real", drc.ToString());

            xmlWriter.WriteElementString("key", "AudioTrackDescription");
            xmlWriter.WriteElementString("string", desc);

            xmlWriter.WriteEndElement();
        }
        #endregion
    }
}