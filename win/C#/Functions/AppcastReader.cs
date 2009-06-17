/*  RssReader.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Xml;
using System.Text.RegularExpressions;

namespace Handbrake.Functions
{
    public class AppcastReader
    {
        XmlDocument rssDoc;
        XmlNode nodeRss;
        XmlNode nodeChannel;
        XmlNode nodeItem;
        private Uri hb_description;
        private string hb_version;
        private string hb_build;
        private string hb_file;

        /// <summary>
        /// Get the build information from the required appcasts.
        /// This must be run before calling any of the public return functions.
        /// </summary>
        public void getInfo()
        {
            // Get the correct Appcast and set input.
            if (Properties.Settings.Default.hb_build.ToString().EndsWith("1"))
                readRss(new XmlTextReader(Properties.Settings.Default.appcast_unstable));
            else 
                readRss(new XmlTextReader(Properties.Settings.Default.appcast));

            string input = nodeItem.InnerXml;

            // Regular Expressions
            Match ver = Regex.Match(input, @"sparkle:version=""([0-9]*)\""");
            Match verShort = Regex.Match(input, @"sparkle:shortVersionString=""([0-9].[0-9].[0-9]*)\""");

            if (nodeItem != null)
            {
                hb_build = ver.ToString().Replace("sparkle:version=", "").Replace("\"", "");
                hb_version = verShort.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");
                hb_file = nodeItem["windows"].InnerText;
                hb_description = new Uri(nodeItem["sparkle:releaseNotesLink"].InnerText);
            }

        }

        /// <summary>
        /// Read the RSS file.
        /// </summary>
        /// <param name="rssReader"></param>
        private void readRss(XmlReader rssReader)
        {
            rssDoc = new XmlDocument();
            rssDoc.Load(rssReader);

            for (int i = 0; i < rssDoc.ChildNodes.Count; i++)
            {
                if (rssDoc.ChildNodes[i].Name == "rss")
                    nodeRss = rssDoc.ChildNodes[i];
            }

            for (int i = 0; i < nodeRss.ChildNodes.Count; i++)
            {
                if (nodeRss.ChildNodes[i].Name == "channel")
                    nodeChannel = nodeRss.ChildNodes[i];
            }

            for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
            {
                if (nodeChannel.ChildNodes[i].Name == "item")
                    nodeItem = nodeChannel.ChildNodes[i];
            }
        }

        /// <summary>
        /// Get Information about an update to HandBrake
        /// </summary>
        /// <returns></returns>
        public System.Uri descriptionUrl()
        {
            return hb_description;
        }

        /// <summary>
        /// Get HandBrake's version from the appcast.xml file.
        /// </summary>
        /// <returns></returns>
        public string version()
        {
            return hb_version;
        }

        /// <summary>
        /// Get HandBrake's Build from the appcast.xml file.
        /// </summary>
        /// <returns></returns>
        public string build()
        {
            return hb_build;
        }

        /// <summary>
        /// Get's the URL for update file.
        /// </summary>
        /// <returns></returns>
        public string downloadFile()
        {
            return hb_file;
        }
    }
}