/*  RssReader.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Text.RegularExpressions;

namespace Handbrake.Functions
{
    class AppcastReader
    {
        XmlTextReader rssReader;
        XmlDocument rssDoc;
        XmlNode nodeRss;
        XmlNode nodeChannel;
        XmlNode nodeItem;
        private string hb_versionInfo;
        private string hb_version;
        private string hb_build;
        private string hb_file;

        // Rss Reading Code.
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


        // Get's the information required out the RSS file.
        private void getInfo()
        {
            Match ver;
            int unstable_build = 0;
            string input;

            // Check the stable appcast and get the build nuber
            rssReader = new XmlTextReader(Properties.Settings.Default.appcast);
            readRss(rssReader);
            input = nodeItem.InnerXml;
            ver = Regex.Match(input, @"sparkle:version=""([0-9]*)\""");
            int stable_build = int.Parse(ver.ToString().Replace("sparkle:version=", "").Replace("\"", ""));

            // If the pref to enable unstable appcast checking is enabled    OR
            // this is a snapshot release, 
            // then check the unstable appcast.
            if (Properties.Settings.Default.checkSnapshot == "Checked" || Properties.Settings.Default.hb_build.ToString().EndsWith("1"))
            {
                // Get the stable build
                rssReader = new XmlTextReader(Properties.Settings.Default.appcast_unstable);
                readRss(rssReader);
                input = nodeItem.InnerXml;
                ver = Regex.Match(input, @"sparkle:version=""([0-9]*)\""");
                unstable_build = int.Parse(ver.ToString().Replace("sparkle:version=", "").Replace("\"", ""));
            }

            if (stable_build >= unstable_build)
                rssReader = new XmlTextReader(Properties.Settings.Default.appcast);
            else
                rssReader = new XmlTextReader(Properties.Settings.Default.appcast_unstable);

            // Get the Version Information
            hb_versionInfo = nodeItem["description"].InnerText;

            // Get the version
            string inputNode = nodeItem.InnerXml;
            ver = Regex.Match(inputNode, @"sparkle:shortVersionString=""([0-9].[0-9].[0-9]*)\""");
            hb_version = ver.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");

            ver = Regex.Match(inputNode, @"sparkle:version=""([0-9]*)\""");
            hb_build = ver.ToString().Replace("sparkle:version=", "").Replace("\"", "");

            // Get the update file
            hb_file = nodeItem["windows"].InnerText;
        }

        /// <summary>
        /// Get Information about an update to HandBrake
        /// </summary>
        /// <returns></returns>
        public string versionInfo()
        {
            getInfo();
            return hb_versionInfo;
        }

        /// <summary>
        /// Get HandBrake's version from the appcast.xml file.
        /// </summary>
        /// <returns></returns>
        public string version()
        {
            getInfo();
            return hb_version;
        }

        /// <summary>
        /// Get HandBrake's Build from the appcast.xml file.
        /// </summary>
        /// <returns></returns>
        public string build()
        {
            getInfo();
            return hb_build;
        }

        /// <summary>
        /// Get's the URL for update file.
        /// </summary>
        /// <returns></returns>
        public string downloadFile()
        {
            getInfo();
            return hb_file;
        }
    }
}