// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PlistUtility.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Plist Preset Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System.Text;
    using System.Xml;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// Plist Preset Converter
    /// </summary>
    public class PlistUtility
    {
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
        /// <param name="build">
        /// The build.PictureModulusPictureModulus
        /// </param>
        public static void Export(string path, Preset preset, string build)
        {
            if (string.IsNullOrEmpty(path))
            {
                return;
            }

            EncodeTask parsed = new EncodeTask(preset.Task);
            using (XmlTextWriter xmlWriter = new XmlTextWriter(path, Encoding.UTF8) { Formatting = Formatting.Indented })
            {
                // Header
                xmlWriter.WriteStartDocument();
                xmlWriter.WriteDocType(
                    "plist", "-//Apple//DTD PLIST 1.0//EN", @"http://www.apple.com/DTDs/PropertyList-1.0.dtd", null);

                xmlWriter.WriteStartElement("plist");
                xmlWriter.WriteStartElement("array");

                // Add New Preset Here. Can write multiple presets here if required in future.
                WritePreset(xmlWriter, parsed, preset, build);

                // Footer
                xmlWriter.WriteEndElement();
                xmlWriter.WriteEndElement();

                xmlWriter.WriteEndDocument();

                // Closeout
                xmlWriter.Close();
            }
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
        /// <param name="build">
        /// The build.
        /// </param>
        private static void WritePreset(XmlTextWriter xmlWriter, EncodeTask parsed, Preset preset, string build)
        {
            xmlWriter.WriteStartElement("dict");
            AudioListArrayDict(xmlWriter, parsed);
            AddEncodeSettings(xmlWriter, parsed, preset, build);

            xmlWriter.WriteEndElement();
        }

        /// <summary>
        /// The get null bool value.
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <returns>
        /// The System.String.
        /// </returns>
        private static string getNullBoolValue(bool? value)
        {
            if (!value.HasValue)
            {
                return "1";
            }

            return value.Value ? "1" : "0";
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
        /// <param name="build">
        /// The build.
        /// </param>
        private static void AddEncodeSettings(XmlTextWriter xmlWriter, EncodeTask parsed, Preset preset, string build)
        {
            AddEncodeElement(xmlWriter, "AudioAllowAACPass", "integer", getNullBoolValue(parsed.AllowedPassthruOptions.AudioAllowAACPass));
            AddEncodeElement(xmlWriter, "AudioAllowAC3Pass", "integer", getNullBoolValue(parsed.AllowedPassthruOptions.AudioAllowAC3Pass));
            AddEncodeElement(xmlWriter, "AudioAllowDTSHDPass", "integer", getNullBoolValue(parsed.AllowedPassthruOptions.AudioAllowDTSHDPass));
            AddEncodeElement(xmlWriter, "AudioAllowDTSPass", "integer", getNullBoolValue(parsed.AllowedPassthruOptions.AudioAllowDTSPass));
            AddEncodeElement(xmlWriter, "AudioAllowMP3Pass", "integer", getNullBoolValue(parsed.AllowedPassthruOptions.AudioAllowMP3Pass));
            AddEncodeElement(xmlWriter, "AudioEncoderFallback", "string", EnumHelper<AudioEncoder>.GetDisplay(parsed.AllowedPassthruOptions.AudioEncoderFallback));

            AddEncodeElement(xmlWriter, "ChapterMarkers", "integer", parsed.IncludeChapterMarkers ? "1" : "0");
            AddEncodeElement(xmlWriter, "Default", "integer", "0");
            AddEncodeElement(xmlWriter, "FileFormat", "string", (parsed.OutputFormat == OutputFormat.Mp4 || parsed.OutputFormat == OutputFormat.M4V) ? "MP4 file" : "MKV file");
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
                    AddEncodeElement(xmlWriter, "PictureDecomb", "integer", "2");
                    AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", string.Empty);
                    break;
                case Decomb.Fast:
                    AddEncodeElement(xmlWriter, "PictureDecomb", "integer", "3");
                    AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", string.Empty);
                    break;
                case Decomb.Bob:
                    AddEncodeElement(xmlWriter, "PictureDecomb", "integer", "4");
                    AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", string.Empty);
                    break;
                case Decomb.Custom:
                    AddEncodeElement(xmlWriter, "PictureDecomb", "integer", "1");
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
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "2");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                case Deinterlace.Slow:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "3");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                case Deinterlace.Slower:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "4");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                case Deinterlace.Bob:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "5");
                    AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", string.Empty);
                    break;
                case Deinterlace.Custom:
                    AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", "1");
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
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "2");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
                    break;
                case Denoise.Medium:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "3");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
                    break;
                case Denoise.Strong:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "4");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", string.Empty);
                    break;
                case Denoise.Custom:
                    AddEncodeElement(xmlWriter, "PictureDenoise", "integer", "1");
                    AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", parsed.CustomDenoise);
                    break;
            }

            int detelecine = 0;
            switch (parsed.Detelecine)
            {
                case Detelecine.Off:
                    detelecine = 0;
                    break;
                case Detelecine.Default:
                    detelecine = 2;
                    break;
                case Detelecine.Custom:
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
            AddEncodeElement(xmlWriter, "PresetBuildNumber", "string", build);
            AddEncodeElement(xmlWriter, "PresetDescription", "string", "No Description");
            AddEncodeElement(xmlWriter, "PresetName", "string", preset.Name);
            AddEncodeElement(xmlWriter, "Type", "integer", "1"); // 1 is user preset, 0 is built in

            // Preset Settings
            AddEncodeElement(xmlWriter, "UsesMaxPictureSettings", "integer", (parsed.MaxWidth != 0 || parsed.MaxHeight != 0) ? "1" : "0");
            AddEncodeElement(xmlWriter, "UsesPictureFilters", "integer", "1");
            AddEncodeElement(xmlWriter, "UsesPictureSettings", "integer", "2");

            // Video Settings
            AddEncodeElement(xmlWriter, "VideoAvgBitrate", "string", parsed.VideoBitrate.ToString());
            AddEncodeElement(xmlWriter, "VideoEncoder", "string", EnumHelper<VideoEncoder>.GetDisplay(parsed.VideoEncoder));
            AddEncodeElement(xmlWriter, "VideoFramerate", "string", parsed.Framerate == null ? "Same as source" : parsed.Framerate.ToString());
            AddEncodeElement(xmlWriter, "VideoFramerateMode", "string", parsed.FramerateMode.ToString().ToLower());
            AddEncodeElement(xmlWriter, "VideoGrayScale", "integer", parsed.Grayscale ? "1" : "0");
            AddEncodeElement(xmlWriter, "VideoQualitySlider", "real", parsed.Quality.ToString());
            AddEncodeElement(xmlWriter, "h264Level", "string", parsed.H264Level);
            AddEncodeElement(xmlWriter, "x264OptionExtra", "string", parsed.AdvancedEncoderOptions);
            AddEncodeElement(xmlWriter, "x264Preset", "string", parsed.X264Preset.ToString().ToLower());
            AddEncodeElement(xmlWriter, "h264Profile", "string", parsed.H264Profile.ToString().ToLower());
            string tune = parsed.X264Tune.ToString().ToLower();
            if (parsed.FastDecode)
            {
                tune = tune == "none" ? "fastdecode" : tune + ",fastdecode";
            }
            AddEncodeElement(xmlWriter, "x264Tune", "string", tune);
            AddEncodeElement(xmlWriter, "x264UseAdvancedOptions", "integer", parsed.ShowAdvancedTab ? "1" : "0");
            

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
            xmlWriter.WriteElementString("string", EnumHelper<AudioEncoder>.GetDisplay(audioTrack.Encoder));

            xmlWriter.WriteElementString("key", "AudioMixdown");
            xmlWriter.WriteElementString("string", EnumHelper<Mixdown>.GetDisplay(audioTrack.MixDown));

            xmlWriter.WriteElementString("key", "AudioSamplerate");
            xmlWriter.WriteElementString("string", audioTrack.SampleRate.ToString().Replace("0", "Auto"));

            xmlWriter.WriteElementString("key", "AudioTrack");
            xmlWriter.WriteElementString("integer", audioTrack.Track.ToString());

            xmlWriter.WriteElementString("key", "AudioTrackDRCSlider");
            xmlWriter.WriteElementString("real", audioTrack.DRC.ToString());

            xmlWriter.WriteElementString("key", "AudioTrackDescription");
            xmlWriter.WriteElementString("string", "Unknown");

            xmlWriter.WriteElementString("key", "AudioTrackGainSlider");
            xmlWriter.WriteElementString("real", audioTrack.Gain.ToString());

            xmlWriter.WriteEndElement();
        }
        #endregion
    }
}