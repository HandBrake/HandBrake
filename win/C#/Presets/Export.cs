/*  Export.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Text;
using System.Xml;

namespace Handbrake.Presets
{
    class Export
    {
        // WARNING This file is not complete!!!!!!

        // TODO 
        // 1. Hookup all the widgets in the GUI so it exports real values.
        // 2. Check for any missing key/value pairs
        // 3. Test with MacGui / Cleanup code  / Retest

        private XmlTextWriter writer;

        public void ExportMacPreset(string path)
        {
            writer = new XmlTextWriter("C:\\test.xml", Encoding.UTF8);

            // Header
            writer.WriteStartDocument();
            writer.WriteDocType("plist", "-//Apple//DTD PLIST 1.0//EN", @"http://www.apple.com/DTDs/PropertyList-1.0.dtd", null);

            writer.WriteStartElement("plist");
            writer.WriteStartElement("array");
            
            // Add New Preset Here. Can write multiple presets here if required in future.
            WritePreset();

            // Footer
            writer.WriteEndElement();
            writer.WriteEndElement();

            writer.WriteEndDocument();

            // Closeout
            writer.Close();
        }

        // Primary function for writing dict.
        private void WritePreset()
        {
            writer.WriteStartElement("dict");
            AudioListArrayDict();
            AddEncodeSettings();

            writer.WriteEndElement();
        }

        // Add all the encode settings to the preset
        private void AddEncodeSettings()
        {
            AddEncodeElement("ChapterMarkers", "integer", "");
            AddEncodeElement("Default", "integer", "");
            AddEncodeElement("FileFormat", "String", "");
            AddBooleanElement("Folder", true);
            AddEncodeElement("Mp4HttpOptimize", "integer", "");
            AddEncodeElement("Mp4LargeFile", "integer", "");
            AddEncodeElement("Mp4iPodCompatible", "integer", "");
            AddEncodeElement("PictureAutoCrop", "integer", "");
            AddEncodeElement("PictureBottomCrop", "integer", "");
            AddEncodeElement("PictureDeblock", "integer", "");
            AddEncodeElement("PictureDecomb", "integer", "");
            AddEncodeElement("PictureDecombCustom", "string", "");
            AddEncodeElement("PictureDecombDeinterlace", "integer", "");
            AddEncodeElement("PictureDeinterlace", "integer", "");
            AddEncodeElement("PictureDeinterlaceCustom", "string", "");
            AddEncodeElement("PictureDenoise", "integer", "");
            AddEncodeElement("PictureDenoiseCustom", "string", "");
            AddEncodeElement("PictureDetelecine", "integer", "");
            AddEncodeElement("PictureDetelecineCustom", "string", "");
            AddEncodeElement("PictureHeight", "integer", "");
            AddEncodeElement("PictureKeepRatio", "integer", "");
            AddEncodeElement("PictureLeftCrop", "integer", "");
            AddEncodeElement("PicturePAR", "integer", "");
            AddEncodeElement("PictureRightCrop", "integer", "");
            AddEncodeElement("PictureTopCrop", "integer", "");
            AddEncodeElement("PictureWidth", "integer", "");
            AddEncodeElement("PresetBuildNumber", "string", "");
            AddEncodeElement("PresetDescription", "string", "");
            AddEncodeElement("PresetName", "string", "");
            AddEncodeElement("Type", "integer", "");
            AddEncodeElement("UsesMaxPictureSettings", "integer", "");
            AddEncodeElement("UsesPictureFilters", "integer", "");
            AddEncodeElement("UsesPictureSettings", "integer", "");
            AddEncodeElement("VideoAvgBitrate", "string", "");
            AddEncodeElement("VideoEncoder", "string", "");
            AddEncodeElement("VideoFramerate", "string", "");
            AddEncodeElement("VideoGrayScale", "integer", "");
            AddEncodeElement("VideoQualitySlider", "real", "");
            AddEncodeElement("VideoQualityType", "integer", "");
            AddEncodeElement("VideoTargetSize", "string", "");
            AddEncodeElement("VideoTurboTwoPass", "integer", "");
            AddEncodeElement("VideoTwoPass", "integer", "");
            AddEncodeElement("x264Option", "string", "");
        }

        // Add Preset setting + value
        private void AddBooleanElement(string keyName, Boolean value)
        {
            writer.WriteStartElement("key");
            writer.WriteString(keyName);
            writer.WriteEndElement();
            if (value)
            {
                writer.WriteStartElement("true");
                writer.WriteEndElement();
            }
            else
            {
                writer.WriteStartElement("false");
                writer.WriteEndElement();
            }
        }
        private void AddEncodeElement(string keyName, string type, string value)
        {
            writer.WriteElementString("key", keyName);
            writer.WriteElementString(type, value);
        }

        // Add List of audio tracks
        private void AudioListArrayDict()
        {
            writer.WriteStartElement("key");
            writer.WriteString("AudioList");
            writer.WriteEndElement();

            writer.WriteStartElement("array");
            AddAudioItem();
            AddAudioItem();
            writer.WriteEndElement();
        }
        private void AddAudioItem()
        {
            int bitrate = 448;
            string encoder = "AC3 Passthru";
            string mixdown = "AC3 Passthru";
            string sample = "Auto";
            int track = 1;
            double drc = 0.0;
            string desc = "English (AC3) (5.1 ch)";

            writer.WriteStartElement("dict");

            writer.WriteElementString("key", "AudioBitrate");
            writer.WriteElementString("string", bitrate.ToString());

            writer.WriteElementString("key", "AudioEncoder");
            writer.WriteElementString("string", encoder);

            writer.WriteElementString("key", "AudioMixdown");
            writer.WriteElementString("string", mixdown);

            writer.WriteElementString("key", "AudioSamplerate");
            writer.WriteElementString("string", sample);

            writer.WriteElementString("key", "AudioTrack");
            writer.WriteElementString("integer", track.ToString());

            writer.WriteElementString("key", "AudioTrackDRCSlider");
            writer.WriteElementString("real", drc.ToString());

            writer.WriteElementString("key", "AudioTrackDescription");
            writer.WriteElementString("string", desc);

            writer.WriteEndElement();
        }
    }
}