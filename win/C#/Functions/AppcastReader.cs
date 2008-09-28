/*  RssReader.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.IO;
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
        private string hb_description;
        private string hb_version;
        private string hb_build;
        private string hb_file;

        /// <summary>
        /// Get the build information from the required appcasts.
        /// This must be run before calling any of the public return functions.
        /// </summary>
        public void getInfo()
        {
            Match ver;
            int stable_build, unstable_build = 0;
            string input, unstable_description = "", stable_description, unstable_version = "", stable_version;
            string stable_file, unstable_file = "";

            // Check the stable appcast and get the stable build number
            readRss(new XmlTextReader(Properties.Settings.Default.appcast));
            input = nodeItem.InnerXml;
            ver = Regex.Match(input, @"sparkle:version=""([0-9]*)\""");
            stable_build = int.Parse(ver.ToString().Replace("sparkle:version=", "").Replace("\"", ""));
            ver = Regex.Match(input, @"sparkle:shortVersionString=""([0-9].[0-9].[0-9]*)\""");
            stable_version = ver.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");
            stable_description = nodeItem["description"].InnerText;
            stable_file = nodeItem["windows"].InnerText;

            // If this is a snapshot release, or the user wants to check for snapshot releases
            if (Properties.Settings.Default.checkSnapshot == "Checked" || Properties.Settings.Default.hb_build.ToString().EndsWith("1"))
            {
                // Get the stable build
                readRss(new XmlTextReader(Properties.Settings.Default.appcast_unstable));
                input = nodeItem.InnerXml;
                ver = Regex.Match(input, @"sparkle:version=""([0-9]*)\""");
                unstable_build = int.Parse(ver.ToString().Replace("sparkle:version=", "").Replace("\"", ""));
                ver = Regex.Match(input, @"sparkle:shortVersionString=""([0-9a-zA-Z.]*)\""");
                unstable_version = ver.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");
                unstable_description = nodeItem["description"].InnerText;
                unstable_file = nodeItem["windows"].InnerText;
            }


            // Set the global version information
            if (stable_build >= unstable_build)
            {
                hb_description = stable_description;
                hb_version = stable_version;
                hb_build = stable_build.ToString();
                hb_file = stable_file;
            }
            else
            {
                hb_description = unstable_description;
                hb_version = unstable_version;
                hb_build = unstable_build.ToString();
                hb_file = unstable_file;
            }
        }

        /// <summary>
        /// Read the RSS file.
        /// </summary>
        /// <param name="rssReader"></param>
        private void readRss(XmlTextReader rssReader)
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
        public string versionInfo()
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