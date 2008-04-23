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

        private void readRss()
        {
            rssReader = new XmlTextReader(Properties.Settings.Default.appcast);
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

        private string hb_versionInfo;
        private string hb_version;
        private string hb_build;
        private string hb_file;

        public void getInfo()
        {
            readRss();

            // Get the Version Information
            hb_versionInfo = nodeItem["description"].InnerText;

            // Get the version
            string input = nodeItem.InnerXml;
            Match ver;
            if (Properties.Settings.Default.hb_build.ToString().EndsWith("1"))
                ver = Regex.Match(input, @"<cli-unstable>[0-9]* \""[0-9.]*\""");
            else 
                ver = Regex.Match(input, @"<cli-stable>[0-9]* \""[0-9.]*\""");
            string[] hb_ver_find = ver.ToString().Split(' ');
            hb_version = hb_ver_find[1].Replace("\"", "");

            // Get the build number
            input = nodeItem.InnerXml;
            Match build;
            if (Properties.Settings.Default.hb_build.ToString().EndsWith("1"))
                build = Regex.Match(input, @"<cli-unstable>[0-9]*");
            else 
                build = Regex.Match(input, @"<cli-stable>[0-9]*");
            hb_build = build.ToString().Replace("<cli-stable>", "").Replace("<cli-unstable>", "");

            // Get the update file
            hb_file = nodeItem["windows"].InnerText;
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