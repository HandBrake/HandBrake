/*  PlistUtility.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;
    using System.Windows.Forms;
    using System.Xml;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// Plist Preset Converter
    /// </summary>
    public class PlistPresetHandler
    {
        /// <summary>
        /// The User Setting Service
        /// </summary>
        private static IUserSettingService userSettingService = new UserSettingService();

        /**
         * TODO:
         * - Update with the new vfr,pfr,cfr keys
         * - Clean up this code, it's pretty nasty right now.
         **/

        #region Import

        public static EncodeTask Import(string filename)
        {
            XmlNode root = loadFile(filename);
            if (root == null) return null;

            // We'll query a query parser object and use it's public var structures to store all the data.
            // This will allow the preset loader logic to be used instead of writing custom logic just for this file.
            EncodeTask parsed = new EncodeTask();
            string qualityMode = string.Empty;

            #region Get a List of Audio Track Objects
            XmlNode audioListDict = root.ChildNodes[2].ChildNodes[0].FirstChild.ChildNodes[1];
            List<AudioTrack> audioTracks = new List<AudioTrack>();

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
                            track.Bitrate = int.Parse(value);
                            break;
                        case "AudioEncoder":
                            track.Encoder = Converters.GetAudioEncoder(value.Trim());
                            break;
                        case "AudioMixdown":
                            track.MixDown = Converters.GetAudioMixDown(value.Trim());
                            break;
                        case "AudioSamplerate":
                            track.SampleRate = value == "Auto" ? 0 : double.Parse(value);
                            break;
                        case "AudioTrack":
                           //track.SourceTrack = value;
                            break;
                        case "AudioTrackDRCSlider":
                            track.DRC = double.Parse(value);
                            break;
                    }
                }
                audioTracks.Add(track);
                parsed.AudioTracks = audioTracks;
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
                        parsed.OutputFormat = Converters.GetFileFormat(value);
                        break;
                    case "Mp4HttpOptimize":
                        parsed.OptimizeMP4 = value == "1";
                        break;
                    case "Mp4LargeFile":
                        parsed.IPod5GSupport = value == "1";
                        break;
                    case "Mp4iPodCompatible":
                        parsed.IPod5GSupport = value == "1";
                        break;

                        // Picture Settings
                    case "PictureAutoCrop":
                        // Not used
                        break;
                    case "PictureTopCrop":
                        parsed.Cropping.Top = int.Parse(value);
                        break;
                    case "PictureBottomCrop":
                        parsed.Cropping.Bottom = int.Parse(value);
                        break;
                    case "PictureLeftCrop":
                        parsed.Cropping.Left = int.Parse(value);
                        break;
                    case "PictureRightCrop":
                        parsed.Cropping.Right = int.Parse(value);
                        break;
                    case "PictureHeight":
                        parsed.Height = int.Parse(value);
                        break;
                    case "PictureWidth":
                        parsed.Width = int.Parse(value);
                        break;
                    case "PictureKeepRatio":
                        parsed.KeepDisplayAspect = value == "1";
                        break;
                    case "PicturePAR":
                        switch (value)
                        {
                            case "0":
                                parsed.Anamorphic = Anamorphic.None;
                                break;
                            default:
                                parsed.Anamorphic = Anamorphic.Strict;
                                break;
                            case "2":
                                parsed.Anamorphic = Anamorphic.Loose;
                                break;
                            case "3":
                                parsed.Anamorphic = Anamorphic.Custom;
                                break;
                        }
                        break;

                        // Filters
                    case "PictureDeblock":
                        parsed.Deblock = int.Parse(value);
                        break;
                    case "PictureDecomb":
                        parsed.Decomb = Decomb.Off;
                        // Don't place custom here as it's handled in the filter panel
                        if (value == "2")
                        {
                            parsed.Decomb = Decomb.Default;
                        }
                        break;
                    case "PictureDecombCustom":
                        if (value != string.Empty)
                        {
                            parsed.CustomDecomb = value;
                        }
                        break;
                    case "PictureDecombDeinterlace":
                        // Not Used
                        break;
                    case "PictureDeinterlace":
                        switch (value)
                        {
                            case "0":
                                parsed.Deinterlace = Deinterlace.Off;
                                break;
                                // Don't place custom here as it's handled in the filter panel
                            case "2":
                                parsed.Deinterlace = Deinterlace.Fast;
                                break;
                            case "3":
                                parsed.Deinterlace = Deinterlace.Slow;
                                break;
                            case "4":
                                parsed.Deinterlace = Deinterlace.Slower;
                                break;
                        }
                        break;
                    case "PictureDeinterlaceCustom":
                        if (value != string.Empty)
                        {
                            parsed.CustomDeinterlace = value;
                        }
                        break;
                    case "PictureDenoise":
                        switch (value)
                        {
                            case "0":
                                parsed.Denoise = Denoise.Off;
                                break;
                                // Don't place custom here as it's handled in the filter panel
                            case "2":
                                parsed.Denoise = Denoise.Weak;
                                break;
                            case "3":
                                parsed.Denoise = Denoise.Medium;
                                break;
                            case "4":
                                parsed.Denoise = Denoise.Strong;
                                break;
                        }

                        break;
                    case "PictureDenoiseCustom":
                        if (value != string.Empty)
                        {
                            parsed.CustomDenoise = value;
                        }
                        break;
                    case "PictureDetelecine":
                        parsed.Detelecine = Detelecine.Off;
                        if (value == "1")
                        {
                            parsed.Detelecine = Detelecine.Default;
                        }
                        break;
                    case "PictureDetelecineCustom":
                        if (value != string.Empty)
                        {
                            parsed.CustomDetelecine = value;
                        }
                        break;

                        // Video Tab
                    case "VideoAvgBitrate":
                        if (!string.IsNullOrEmpty(value))
                        {
                            parsed.VideoBitrate = int.Parse(value);
                        }
                        break;
                    case "VideoEncoder":
                        parsed.VideoEncoder = Converters.GetVideoEncoder(value);
                        break;
                    case "VideoFramerate":

                        if (value == "Same as source")
                        {
                            parsed.Framerate = null;
                        }
                        else if (!string.IsNullOrEmpty(value))
                        {
                            parsed.Framerate = int.Parse(value);
                        }
                        break;
                    case "VideoGrayScale":
                        parsed.Grayscale = value == "1";
                        break;
                    case "VideoQualitySlider":
                        parsed.Quality = double.Parse(value);
                        break;
                    case "VideoQualityType": // The Type of Quality Mode used
                        qualityMode = value;
                        break;
                    case "VideoTurboTwoPass":
                        parsed.TurboFirstPass = value == "1";
                        break;
                    case "VideoTwoPass":
                        parsed.TwoPass = value == "1";
                        break;

                        // Chapter Markers Tab
                    case "ChapterMarkers":
                        parsed.IncludeChapterMarkers = value == "1";
                        break;

                        // Advanced x264 tab
                    case "x264Option":
                        parsed.AdvancedEncoderOptions = value;
                        break;

                        // Preset Information
                    case "PresetBuildNumber":
                        parsed.PresetBuildNumber = int.Parse(value);
                        break;
                    case "PresetDescription":
                        parsed.PresetDescription = value;
                        break;
                    case "PresetName":
                        parsed.PresetName = value;
                        break;
                    case "Type":
                        parsed.Type = value;
                        break;
                    case "UsesMaxPictureSettings":
                        parsed.UsesMaxPictureSettings = value == "1";
                        break;
                    case "UsesPictureFilters":
                        parsed.UsesPictureFilters = value == "1";
                        break;
                    case "UsesPictureSettings":
                        parsed.UsesPictureSettings = value == "1";
                        break;
                }
            }

            // Kill any Quality values we don't need.
            switch (qualityMode)
            {
                case "1": // Avg Bitrate
                    parsed.VideoEncodeRateType = VideoEncodeRateType.AverageBitrate;
                    break;
                case "2": // CQ
                    parsed.VideoEncodeRateType = VideoEncodeRateType.ConstantQuality;
                    break;
            }
            #endregion

            return parsed;
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

        /// <summary>
        /// Export a MacGui style plist preset.
        /// </summary>
        /// <param name="path">
        /// The path.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        public static void Export(string path, Preset preset)
        {
            EncodeTask parsed = QueryParserUtility.Parse(preset.Query);
            XmlTextWriter xmlWriter = new XmlTextWriter(path, Encoding.UTF8) { Formatting = Formatting.Indented };

            // Header
            xmlWriter.WriteStartDocument();
            xmlWriter.WriteDocType("plist", "-//Apple//DTD PLIST 1.0//EN",
                                @"http://www.apple.com/DTDs/PropertyList-1.0.dtd", null);

            xmlWriter.WriteStartElement("plist");
            xmlWriter.WriteStartElement("array");

            // Add New Preset Here. Can write multiple presets here if required in future.
            WritePreset(xmlWriter, parsed, preset);

            // Footer
            xmlWriter.WriteEndElement();
            xmlWriter.WriteEndElement();

            xmlWriter.WriteEndDocument();

            // Closeout
            xmlWriter.Close();
        }

        /// <summary>
        /// Write the Preset to a file
        /// </summary>
        /// <param name="xmlWriter">
        /// The xml writer.
        /// </param>
        /// <param name="parsed">
        /// The parsed.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        private static void WritePreset(XmlTextWriter xmlWriter, EncodeTask parsed, Preset preset)
        {
            xmlWriter.WriteStartElement("dict");
            AudioListArrayDict(xmlWriter, parsed);
            AddEncodeSettings(xmlWriter, parsed, preset);

            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Add the encode settings to the preset
        /// </summary>
        /// <param name="xmlWriter">
        /// The xml writer.
        /// </param>
        /// <param name="parsed">
        /// The parsed.
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        private static void AddEncodeSettings(XmlTextWriter xmlWriter, EncodeTask parsed, Preset preset)
        {
            AddEncodeElement(xmlWriter, "ChapterMarkers", "integer", parsed.IncludeChapterMarkers ? "1" : "0");
            AddEncodeElement(xmlWriter, "Default", "integer", "0");
            AddEncodeElement(xmlWriter, "FileFormat", "string", Converters.GetFileFormat(parsed.OutputFormat) + " file");
            AddBooleanElement(xmlWriter, "Folder", false);
            AddEncodeElement(xmlWriter, "Mp4HttpOptimize", "integer", parsed.OptimizeMP4 ? "1" : "0");
            AddEncodeElement(xmlWriter, "Mp4LargeFile", "integer", parsed.LargeFile ? "1" : "0");
            AddEncodeElement(xmlWriter, "Mp4iPodCompatible", "integer", parsed.IPod5GSupport ? "1" : "0");
            AddEncodeElement(xmlWriter, "PictureAutoCrop", "integer", "1");
            AddEncodeElement(xmlWriter, "PictureBottomCrop", "integer", parsed.Cropping.Bottom.ToString());

            // Filters
            AddEncodeElement(xmlWriter, "PictureDeblock", "integer", parsed.Deblock.ToString());

            switch (parsed.Decomb)
            {
                case Decomb.Off:
                    AddEncodeElement(xmlWriter, "PictureDecomb", "integer", "0");
                    AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", string.Empty);
                    break;
                case Decomb.Default:
                    AddEncodeElement(xmlWriter, "PictureDecomb", "integer", "1");
                    AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", string.Empty);
                    break;
                default:
                    AddEncodeElement(xmlWriter, "PictureDecomb", "integer", "2");
                    AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", parsed.CustomDecomb);
                    break;
            }
            AddEncodeElement(xmlWriter, "PictureDecombDeinterlace", "integer", parsed.Decomb != Decomb.Off ? "0" : "1");

            switch (parsed.Deinterlace)
            {
                case Deinterlace.Off:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "0");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                case Deinterlace.Fast:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "1");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                case Deinterlace.Slow:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "2");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                case Deinterlace.Slower:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "3");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                default:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "4");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", parsed.CustomDeinterlace);
                    break;
            }

            switch (parsed.Denoise)
            {
                case Denoise.Off:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "0");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
                    break;
                case Denoise.Weak:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "1");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
                    break;
                case Denoise.Medium:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "2");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
                    break;
                case Denoise.Strong:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "3");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
                    break;
                default:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "4");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", parsed.CustomDenoise);
                    break;
            }

            int detelecine;
            switch (parsed.Detelecine)
            {
                case Detelecine.Off:
                    detelecine = 0;
                    break;
                case Detelecine.Default:
                    detelecine = 2;
                    break;
                default:
                    detelecine = 1;
                    break;
            }

            AddEncodeElement(xmlWriter, "PictureDetelecine", "integer", detelecine.ToString());
            AddEncodeElement(xmlWriter, "PictureDetelecineCustom", "string", detelecine == 1 ? parsed.CustomDecomb : string.Empty);

            // Picture Settings
            AddEncodeElement(xmlWriter, "PictureHeight", "integer", parsed.Height.ToString());
            AddEncodeElement(xmlWriter, "PictureKeepRatio", "integer", parsed.KeepDisplayAspect ? "1" : "0");
            AddEncodeElement(xmlWriter, "PictureLeftCrop", "integer", parsed.Cropping.Left.ToString());
            AddEncodeElement(xmlWriter, "PictureModulus", "integer", parsed.Modulus.ToString());
            AddEncodeElement(xmlWriter, "PicturePAR", "integer", ((int)parsed.Anamorphic).ToString());
            AddEncodeElement(xmlWriter, "PictureRightCrop", "integer", parsed.Cropping.Right.ToString());
            AddEncodeElement(xmlWriter, "PictureTopCrop", "integer", parsed.Cropping.Top.ToString());
            AddEncodeElement(xmlWriter, "PictureWidth", "integer", parsed.Width.ToString());

            // Preset Information
            AddEncodeElement(xmlWriter, "PresetBuildNumber", "string", userSettingService.GetUserSetting<string>(UserSettingConstants.HandBrakeBuild));
            AddEncodeElement(xmlWriter, "PresetDescription", "string", "No Description");
            AddEncodeElement(xmlWriter, "PresetName", "string", preset.Name);
            AddEncodeElement(xmlWriter, "Type", "integer", "1"); // 1 is user preset, 0 is built in

            // Preset Settings
            AddEncodeElement(xmlWriter, "UsesMaxPictureSettings", "integer", (parsed.MaxWidth != 0 || parsed.MaxHeight != 0) ? "1" : "0");
            AddEncodeElement(xmlWriter, "UsesPictureFilters", "integer", "1");
            AddEncodeElement(xmlWriter, "UsesPictureSettings", "integer", "2");

            // Video Settings
            AddEncodeElement(xmlWriter, "VideoAvgBitrate", "string", parsed.VideoBitrate.ToString());
            AddEncodeElement(xmlWriter, "VideoEncoder", "string", EnumHelper<VideoEncoder>.GetDisplayValue(parsed.VideoEncoder));
            AddEncodeElement(xmlWriter, "VideoFramerate", "string", parsed.Framerate.ToString());
            AddEncodeElement(xmlWriter, "VideFrameratePFR", "integer", parsed.FramerateMode == FramerateMode.PFR ? "1" : "0");
            AddEncodeElement(xmlWriter, "VideoGrayScale", "integer", parsed.Grayscale ? "1" : "0");
            AddEncodeElement(xmlWriter, "VideoQualitySlider", "real", parsed.Quality.ToString());

            int videoQualityType = 0;
            if (parsed.VideoBitrate != null) videoQualityType = 1;
            else if (parsed.Quality != null) videoQualityType = 2;

            AddEncodeElement(xmlWriter, "VideoQualityType", "integer", videoQualityType.ToString());
            AddEncodeElement(xmlWriter, "VideoTargetSize", "string", string.Empty);
            AddEncodeElement(xmlWriter, "VideoTurboTwoPass", "integer", parsed.TurboFirstPass ? "1" : "0");
            AddEncodeElement(xmlWriter, "VideoTwoPass", "integer", parsed.TwoPass ? "1" : "0");

            // x264 string
            AddEncodeElement(xmlWriter, "x264Option", "string", parsed.AdvancedEncoderOptions);
        }

        /// <summary>
        /// Add a boolean element
        /// </summary>
        /// <param name="xmlWriter">
        /// The xml writer.
        /// </param>
        /// <param name="keyName">
        /// The key name.
        /// </param>
        /// <param name="value">
        /// The value.
        /// </param>
        private static void AddBooleanElement(XmlTextWriter xmlWriter, string keyName, bool value)
        {
            xmlWriter.WriteStartElement("key");
            xmlWriter.WriteString(keyName);
            xmlWriter.WriteEndElement();
            xmlWriter.WriteStartElement(value ? "true" : "false");
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Add an encode setting element
        /// </summary>
        /// <param name="xmlWriter">
        /// The xml writer.
        /// </param>
        /// <param name="keyName">
        /// The key name.
        /// </param>
        /// <param name="type">
        /// The type.
        /// </param>
        /// <param name="value">
        /// The value.
        /// </param>
        private static void AddEncodeElement(XmlTextWriter xmlWriter, string keyName, string type, string value)
        {
            xmlWriter.WriteElementString("key", keyName);

            // This is a hack for Apples XML parser. It doesn't understand <integer /> so instead, always set a default value
            // of 0 if the value is empty.
            if (type == "integer" && string.IsNullOrEmpty(value))
            {
                value = "0";
            }
            xmlWriter.WriteElementString(type, value);
        }

        /// <summary>
        /// Add an Audio Track Array Dict
        /// </summary>
        /// <param name="xmlWriter">
        /// The xml writer.
        /// </param>
        /// <param name="parsed">
        /// The parsed.
        /// </param>
        private static void AudioListArrayDict(XmlTextWriter xmlWriter, EncodeTask parsed)
        {
            xmlWriter.WriteStartElement("key");
            xmlWriter.WriteString("AudioList");
            xmlWriter.WriteEndElement();

            xmlWriter.WriteStartElement("array");
            foreach (AudioTrack track in parsed.AudioTracks)
            {
                AddAudioItem(xmlWriter, track);
            }
            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// Add an audio track
        /// </summary>
        /// <param name="xmlWriter">
        /// The xml writer.
        /// </param>
        /// <param name="audioTrack">
        /// The audio track.
        /// </param>
        private static void AddAudioItem(XmlTextWriter xmlWriter, AudioTrack audioTrack)
        {
            xmlWriter.WriteStartElement("dict");

            xmlWriter.WriteElementString("key", "AudioBitrate");
            xmlWriter.WriteElementString("string", audioTrack.Bitrate.ToString());

            xmlWriter.WriteElementString("key", "AudioEncoder");
            xmlWriter.WriteElementString("string", EnumHelper<Enum>.GetDescription(audioTrack.Encoder));

            xmlWriter.WriteElementString("key", "AudioMixdown");
            xmlWriter.WriteElementString("string", EnumHelper<Enum>.GetDescription(audioTrack.MixDown));

            xmlWriter.WriteElementString("key", "AudioSamplerate");
            xmlWriter.WriteElementString("string", audioTrack.SampleRate.ToString());

            xmlWriter.WriteElementString("key", "AudioTrack");
            xmlWriter.WriteElementString("integer", audioTrack.Track.ToString());

            xmlWriter.WriteElementString("key", "AudioTrackDRCSlider");
            xmlWriter.WriteElementString("real", audioTrack.DRC.ToString());

            xmlWriter.WriteElementString("key", "AudioTrackDescription");
            xmlWriter.WriteElementString("string", "Unknown");

            xmlWriter.WriteEndElement();
        }
        #endregion
    }
}