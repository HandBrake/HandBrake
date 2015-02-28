// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PlistFactory.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Plist Preset Converter
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Presets.Factories
{
    using System.Linq;
    using System.Text;
    using System.Xml;

    using HandBrake.ApplicationServices.Services.Encode.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model.Models;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// Plist Preset Converter
    /// </summary>
    public class PlistFactory
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
            AddBooleanElement(xmlWriter, "AudioAllowAACPass", parsed.AllowedPassthruOptions.AudioAllowAACPass);
            AddBooleanElement(xmlWriter, "AudioAllowAC3Pass", parsed.AllowedPassthruOptions.AudioAllowAC3Pass);
            AddBooleanElement(xmlWriter, "AudioAllowDTSHDPass", parsed.AllowedPassthruOptions.AudioAllowDTSHDPass);
            AddBooleanElement(xmlWriter, "AudioAllowDTSPass", parsed.AllowedPassthruOptions.AudioAllowDTSPass);
            AddBooleanElement(xmlWriter, "AudioAllowMP3Pass", parsed.AllowedPassthruOptions.AudioAllowMP3Pass);

            // TODO Update
            AddEncodeElement(xmlWriter, "AudioEncoderFallback", "string", EnumHelper<AudioEncoder>.GetDisplay(parsed.AllowedPassthruOptions.AudioEncoderFallback));

            AddBooleanElement(xmlWriter, "ChapterMarkers", parsed.IncludeChapterMarkers);
            AddEncodeElement(xmlWriter, "Default", "integer", "0");
            AddEncodeElement(xmlWriter, "FileFormat", "string", (parsed.OutputFormat == OutputFormat.Mp4) ? "MP4 file" : "MKV file"); // TODO
            AddBooleanElement(xmlWriter, "Folder", false);
            AddEncodeElement(xmlWriter, "Mp4HttpOptimize", "integer", parsed.OptimizeMP4 ? "1" : "0");
            AddEncodeElement(xmlWriter, "Mp4iPodCompatible", "integer", parsed.IPod5GSupport ? "1" : "0");
            AddEncodeElement(xmlWriter, "PictureAutoCrop", "integer", "1");
            

            // Filters
            AddEncodeElement(xmlWriter, "PictureDeblock", "integer", parsed.Deblock.ToString());

            AddBooleanElement(xmlWriter, "PictureDecombDeinterlace", parsed.Decomb != Decomb.Off);
            AddEncodeElement(xmlWriter, "PictureDecombCustom", "string", parsed.Decomb == Decomb.Custom ? parsed.CustomDecomb : string.Empty);
            AddEncodeElement(xmlWriter, "PictureDecomb", "integer", ((int)parsed.Decomb).ToString());
            AddEncodeElement(xmlWriter, "PictureDeinterlaceCustom", "string", parsed.Deinterlace == Deinterlace.Custom ? parsed.CustomDeinterlace : string.Empty);
            AddEncodeElement(xmlWriter, "PictureDeinterlace", "integer", ((int)parsed.Deinterlace).ToString());


            AddEncodeElement(xmlWriter, "PictureDenoiseFilter", "string", parsed.Denoise.ToString().ToLower());
            AddEncodeElement(xmlWriter, "PictureDenoiseCustom", "string", parsed.DenoisePreset == DenoisePreset.Custom ? parsed.CustomDenoise : string.Empty);
            AddEncodeElement(xmlWriter, "PictureDenoisePreset", "string", parsed.DenoisePreset.ToString().ToLower());
            if (parsed.Denoise == Denoise.NLMeans)
            {
                AddEncodeElement(xmlWriter, "PictureDenoiseTune", "string", parsed.DenoiseTune.ToString().ToLower());
            }

            AddEncodeElement(xmlWriter, "PictureDetelecine", "integer", ((int)parsed.Detelecine).ToString());
            AddEncodeElement(xmlWriter, "PictureDetelecineCustom", "string", parsed.Detelecine == Detelecine.Custom ? parsed.CustomDecomb : string.Empty);

            // Picture Settings
            AddEncodeElement(xmlWriter, "PictureHeight", "integer", parsed.MaxHeight.HasValue ? parsed.MaxHeight.Value.ToString() : parsed.Height.HasValue ? parsed.Height.Value.ToString() : "0");
            AddEncodeElement(xmlWriter, "PictureKeepRatio", "integer", parsed.KeepDisplayAspect ? "1" : "0");          
            AddEncodeElement(xmlWriter, "PictureModulus", "integer", parsed.Modulus.ToString());
            AddEncodeElement(xmlWriter, "PicturePAR", "integer", ((int)parsed.Anamorphic).ToString());
            AddEncodeElement(xmlWriter, "PictureLeftCrop", "integer", parsed.Cropping.Left.ToString());
            AddEncodeElement(xmlWriter, "PictureRightCrop", "integer", parsed.Cropping.Right.ToString());
            AddEncodeElement(xmlWriter, "PictureTopCrop", "integer", parsed.Cropping.Top.ToString());
            AddEncodeElement(xmlWriter, "PictureBottomCrop", "integer", parsed.Cropping.Bottom.ToString());
            AddEncodeElement(xmlWriter, "PictureWidth", "integer", parsed.MaxWidth.HasValue ? parsed.MaxWidth.Value.ToString() : parsed.Width.HasValue ? parsed.Width.Value.ToString() : "0");

            // Preset Information
            AddEncodeElement(xmlWriter, "PresetBuildNumber", "string", build);
            AddEncodeElement(xmlWriter, "PresetDescription", "string", "No Description");
            AddEncodeElement(xmlWriter, "PresetName", "string", preset.Name);
            AddEncodeElement(xmlWriter, "Type", "integer", "1"); // 1 is user preset, 0 is built in

            // Preset Settings
            AddEncodeElement(xmlWriter, "UsesMaxPictureSettings", "integer", (parsed.MaxWidth != 0 || parsed.MaxHeight != 0) ? "1" : "0");
            AddEncodeElement(xmlWriter, "UsesPictureFilters", "integer", "1");
            AddEncodeElement(xmlWriter, "UsesPictureSettings", "integer", "1");

            // Video Settings
            AddEncodeElement(xmlWriter, "VideoAvgBitrate", "string", parsed.VideoBitrate.ToString());
            AddEncodeElement(xmlWriter, "VideoEncoder", "string", EnumHelper<VideoEncoder>.GetDisplay(parsed.VideoEncoder));
            AddEncodeElement(xmlWriter, "VideoFramerate", "string", parsed.Framerate == null ? "Same as source" : parsed.Framerate.ToString());
            AddEncodeElement(xmlWriter, "VideoFramerateMode", "string", parsed.FramerateMode.ToString().ToLower());
            AddBooleanElement(xmlWriter, "VideoGrayScale", parsed.Grayscale);
            AddEncodeElement(xmlWriter, "VideoQualitySlider", "real", parsed.Quality.ToString());

            if (parsed.VideoPreset != null)
                AddEncodeElement(xmlWriter, "VideoPreset", "string", parsed.VideoPreset.ShortName);
            if (parsed.VideoLevel != null)
                AddEncodeElement(xmlWriter, "VideoLevel", "string", parsed.VideoLevel.ShortName);
            if (parsed.VideoProfile != null)
                AddEncodeElement(xmlWriter, "VideoProfile", "string", parsed.VideoProfile.ShortName);
            if (parsed.VideoTunes != null)
                AddEncodeElement(xmlWriter, "VideoTune", "string",  parsed.VideoTunes.Aggregate(string.Empty, (current, item) => string.IsNullOrEmpty(current) ? item.ShortName : "," + item.ShortName));

            AddEncodeElement(xmlWriter, "VideoOptionExtra", "string", parsed.ExtraAdvancedArguments);
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
        private static void AddBooleanElement(XmlTextWriter xmlWriter, string keyName, bool? value)
        {
            xmlWriter.WriteStartElement("key");
            xmlWriter.WriteString(keyName);
            xmlWriter.WriteEndElement();
            xmlWriter.WriteStartElement(value.HasValue ? value.Value ? "true" : "false" : "false");
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