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
        public void GetInfo(string input)
        {
            try
            {
                // Get the correct Appcast and set input.
                XmlNode nodeItem = readRss(new XmlTextReader(new StringReader(input)));
                string result = nodeItem.InnerXml;

                // Regular Expressions
                Match ver = Regex.Match(result, @"sparkle:version=""([0-9]*)\""");
                Match verShort = Regex.Match(result, @"sparkle:shortVersionString=""(([svn]*)([0-9.\s]*))\""");

                Build = ver.ToString().Replace("sparkle:version=", "").Replace("\"", "");
                Version = verShort.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");
                DownloadFile = nodeItem["windows"].InnerText;
                DescriptionUrl = new Uri(nodeItem["sparkle:releaseNotesLink"].InnerText);
            } 
            catch( Exception)
            {
                return;
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

            foreach (XmlNode t in rssDoc.ChildNodes)
            {
                if (t.Name == "rss")
                    nodeRss = t;
            }

            if (nodeRss != null)
                foreach (XmlNode t in nodeRss.ChildNodes)
                {
                    if (t.Name == "channel")
                        nodeChannel = t;
                }

            if (nodeChannel != null)
                foreach (XmlNode t in nodeChannel.ChildNodes)
                {
                    if (t.Name == "item")
                        nodeItem = t;
                }

            return nodeItem;
        }

        /// <summary>
        /// Get Information about an update to HandBrake
        /// </summary>
        public Uri DescriptionUrl { get; set; }

        /// <summary>
        /// Get HandBrake's version from the appcast.xml file.
        /// </summary>
        public string Version { get; set; }

        /// <summary>
        /// Get HandBrake's Build from the appcast.xml file.
        /// </summary>
        public string Build { get; set; }

        /// <summary>
        /// Get's the URL for update file.
        /// </summary>
        public string DownloadFile { get; set; }
    }
}