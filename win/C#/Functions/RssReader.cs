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
            rssReader = new XmlTextReader("http://handbrake.m0k.org/appcast.xml");
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


        public string versionInfo()
        {
            readRss();
            string vinfo = "";
            for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
            {
                if (nodeChannel.ChildNodes[6].Name == "item")
                {
                    nodeItem = nodeChannel.ChildNodes[0];
                    t = readRss();
                    if (nodeItem["title"].InnerText == t)
                    {
                        vinfo = nodeItem["description"].InnerText;
                        break;
                    }
                }
            }

            return vinfo;
         }

        public string version()
        {
            readRss();
            string vinfo = "";
            for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
            {
                if (nodeChannel.ChildNodes[6].Name == "item")
                {
                    nodeItem = nodeChannel.ChildNodes[0];
                    string t = readRss();
                    if (nodeItem["title"].InnerText == t)
                    {
                        string input = nodeItem.InnerXml;
                        Match ver = Regex.Match(input, @"sparkle:shortVersionString=""([0-9].[0-9].[0-9]*)\""");
                        vinfo = ver.ToString().Replace("sparkle:shortVersionString=", "").Replace("\"", "");

                        break;
                    }
                }
            }
            return vinfo;
        }

        public string build()
        {
            readRss();
            string vinfo = "";
            for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
            {
                if (nodeChannel.ChildNodes[6].Name == "item")
                {
                    nodeItem = nodeChannel.ChildNodes[0];
                    string t = readRss();
                    if (nodeItem["title"].InnerText == t)
                    {
                        string input = nodeItem.InnerXml; 
                        Match ver = Regex.Match(input, @"sparkle:version=""([0-9]*)\""");
                        vinfo = ver.ToString().Replace("sparkle:version=", "").Replace("\"", "");
                        break;
                    }
                }
            }
            return vinfo;
        }

        public string downloadFile()
        {
            readRss();
            string file = "";
            for (int i = 0; i < nodeChannel.ChildNodes.Count; i++)
            {
                if (nodeChannel.ChildNodes[6].Name == "item")
                {
                    nodeItem = nodeChannel.ChildNodes[0];
                    t = readRss();
                    if (nodeItem["title"].InnerText == t)
                    {
                        file = nodeItem["windows"].InnerText;
                        break;
                    }
                }
            }

            return file;
         }
    }
}