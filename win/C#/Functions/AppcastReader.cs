/*  RssReader.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Xml;
using System.Text.RegularExpressions;
using System.IO;

namespace Handbrake.Functions
{
    public class AppcastReader
    {
        /// <summary>
        /// Get the build information from the required appcasts. Run before accessing the public vars.
        /// </summary>
        public void getInfo(string input)
        {
            try
            {
                // Get the correct Appcast and set input.
                XmlNode nodeItem = readRss(new XmlTextReader(new StringReader(input)));
                string result = nodeItem.InnerXml;

                // Regular Expressions
                Match ver = Regex.Match(result, @"sparkle:version=""([0-9]*)\""");
                Match verShort = Regex.Match(result, @"sparkle:shortVersionString=""(([svn]*)([0-9.\s]*))\""");

                build = ver.ToString().Replace("sparkle:version=", "").Replace("\"", "");
                version = verShort.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");
                downloadFile = nodeItem["windows"].InnerText;
                descriptionUrl = new Uri(nodeItem["sparkle:releaseNotesLink"].InnerText);
            } catch( Exception)
            {
                // Ignore Error.
            }
        }

        /// <summary>
        /// Read the RSS file.
        /// </summary>
        /// <param name="rssReader"></param>
        private static XmlNode readRss(XmlReader rssReader)
        {
            XmlNode nodeItem = null;
            XmlNode nodeChannel = null;
            XmlNode nodeRss = null;

            XmlDocument rssDoc = new XmlDocument();
            rssDoc.Load(rssReader);

            for (int i = 0; i < rssDoc.ChildNodes.Count; i++)
            {
                if (rssDoc.ChildNodes[i].Name == "rss")
                    nodeRss = rssDoc.ChildNodes[i];
            }

            if (nodeRss != null)
                for (int i = 0; i < nodeRss.ChildNodes.Count; i++)
                {
                    if (nodeRss.ChildNodes[i].Name == "channel")
                        nodeChannel = nodeRss.ChildNodes[i];
                }

            if (nodeChannel != null)
                for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
                {
                    if (nodeChannel.ChildNodes[i].Name == "item")
                        nodeItem = nodeChannel.ChildNodes[i];
                }

            return nodeItem;
        }

        /// <summary>
        /// Get Information about an update to HandBrake
        /// </summary>
        public Uri descriptionUrl { get; set; }

        /// <summary>
        /// Get HandBrake's version from the appcast.xml file.
        /// </summary>
        public string version { get; set; }

        /// <summary>
        /// Get HandBrake's Build from the appcast.xml file.
        /// </summary>
        public string build { get; set; }

        /// <summary>
        /// Get's the URL for update file.
        /// </summary>
        public string downloadFile { get; set; }
    }
}