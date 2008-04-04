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
    class RssReader
    {
        XmlTextReader rssReader;
        XmlDocument rssDoc;
        XmlNode nodeRss;
        XmlNode nodeChannel;
        XmlNode nodeItem;
        string t;

        private string readRss()
        {
            rssReader = new XmlTextReader("http://handbrake.fr/appcast.xml");
            rssDoc = new XmlDocument();
            rssDoc.Load(rssReader);

            for (int i = 0; i < rssDoc.ChildNodes.Count; i++)
            {
                if (rssDoc.ChildNodes[i].Name == "rss")
                {
                    nodeRss = rssDoc.ChildNodes[i];
                }
            }

            for (int i = 0; i < nodeRss.ChildNodes.Count; i++)
            {
                if (nodeRss.ChildNodes[i].Name == "channel")
                {
                    nodeChannel = nodeRss.ChildNodes[i];
                }
            }

            string latestTitle = "";
            for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
            {
 
                if (nodeChannel.ChildNodes[i].Name == "item")
                {
                    nodeItem = nodeChannel.ChildNodes[i];
                    latestTitle = nodeItem["title"].InnerText;
                }
            }
            return latestTitle;
        }

        private string hb_versionInfo;
        private string hb_version;
        private string hb_build;
        private string hb_file;

        public void getInfo()
        {
            readRss();
            for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
            {
                if (nodeChannel.ChildNodes[6].Name == "item")
                {
                    nodeItem = nodeChannel.ChildNodes[0];
                    t = readRss();
                    if (nodeItem["title"].InnerText == t)
                    {
                        // Get the Version Information
                        hb_versionInfo = nodeItem["description"].InnerText;

                        // Get the version
                        string input = nodeItem.InnerXml;
                        Match ver = Regex.Match(input, @"sparkle:shortVersionString=""([0-9].[0-9].[0-9]*)\""");
                        hb_version = ver.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");

                        // Get the build number
                        input = nodeItem.InnerXml;
                        ver = Regex.Match(input, @"sparkle:version=""([0-9]*)\""");
                        hb_build = ver.ToString().Replace("sparkle:version=", "").Replace("\"", "");

                        // Get the update file
                        hb_file = nodeItem["windows"].InnerText;

                    }
                }
            }
         }

        public string versionInfo()
        {
            getInfo();
            return hb_versionInfo;
        }

        public string version()
        {
            getInfo();
            return hb_version;
        }

        public string build()
        {
            getInfo();
            return hb_build;
        }

        public string downloadFile()
        {
            getInfo();
            return hb_file;
        }
    }
}