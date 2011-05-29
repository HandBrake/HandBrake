/*  PlistHelper.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Xml;

    /// <summary>
    /// A Plist Helper
    /// </summary>
    public class PlistHelper
    {
        /// <summary>
        /// Get the Audio Track XML Nodes from a plist preset file
        /// </summary>
        /// <param name="filename">
        /// A Plist preset file
        /// </param>
        /// <returns>
        /// A List of XmlNodes which contain audio tracks
        /// </returns>
        public static List<XmlNode> GetAudioTracks(string filename)
        {
            // Data Store
            List<XmlNode> audioTracks = new List<XmlNode>();

            // Load the file and get the Root note, or return if none found
            XmlNode root = LoadFile(filename);
            if (root == null)
            {
                return null;
            }

            // Get the Audio Tracks
            XmlNode audioListDict = root.ChildNodes[2].ChildNodes[0].FirstChild.ChildNodes[1];
            for (int i = 0; i < audioListDict.ChildNodes.Count; i++)
            {
                XmlNode audioChannel = audioListDict.ChildNodes[i]; // <dict>
                audioTracks.Add(audioChannel);
            }

            return audioTracks;
        }

        /// <summary>
        /// Get the settings portion of a presets file
        /// </summary>
        /// <param name="filename">
        /// A Plist preset file
        /// </param>
        /// <returns>
        /// A Dictionary of plist key against an XML node containing the setting.
        /// </returns>
        public static Dictionary<string, XmlNode> GetPresetSettings(string filename)
        {
            // Data Store
            Dictionary<string, XmlNode> settings = new Dictionary<string, XmlNode>();

            // Load the file and get the Root note, or return if none found
            XmlNode root = LoadFile(filename);
            if (root == null)
            {
                return null;
            }

            // Get the Preset Settings, starting from the 3rd node (skipping the audio notes we've just prcessed)
            XmlNode presetSettings = root.ChildNodes[2].ChildNodes[0].FirstChild;
            for (int i = 2; i < presetSettings.ChildNodes.Count; i += 2)
            {
                string key = presetSettings.ChildNodes[i].InnerText;
                XmlNode node = presetSettings.ChildNodes[i + 1];
                settings.Add(key, node);
            }

            return settings;
        }

        /// <summary>
        /// Load a Plist Preset file and return it as an XML document
        /// </summary>
        /// <param name="filename">
        /// A Plist preset file
        /// </param>
        /// <returns>
        /// An XML document
        /// </returns>
        private static XmlNode LoadFile(string filename)
        {
            try
            {
                if (!File.Exists(filename))
                {
                    return null;
                }

                StreamReader sr = File.OpenText(filename);
                string fromfile = string.Empty;
                int fileChar;
                while ((fileChar = sr.Read()) != -1)
                {
                    fromfile += Convert.ToChar(fileChar);
                }

                XmlDocument doc = new XmlDocument();
                doc.LoadXml(fromfile);

                XmlNode root = doc;
                if (!root.HasChildNodes)
                {
                    return null;
                }

                return root;
            }
            catch (Exception)
            {
                return null;
            }
        }
    }
}
