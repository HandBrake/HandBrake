// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AppcastReader.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Appcast Reader - Used for parsing HandBrakes update file
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.IO;
    using System.Text.RegularExpressions;
    using System.Xml;

    /// <summary>
    /// Appcast Reader - Used for parsing HandBrakes update file
    /// </summary>
    public class AppcastReader
    {
        /// <summary>
        /// Gets Information about an update to HandBrake
        /// </summary>
        public Uri DescriptionUrl { get; private set; }

        /// <summary>
        /// Gets HandBrake's version from the appcast.xml file.
        /// </summary>
        public string Version { get; private set; }

        /// <summary>
        /// Gets HandBrake's Build from the appcast.xml file.
        /// </summary>
        public string Build { get; private set; }

        /// <summary>
        /// Gets the URL for update file.
        /// </summary>
        public string DownloadFile { get; private set; }

        /// <summary>
        /// Gets the Signature for verifying the download completed correctly.
        /// </summary>
        public string Signature { get; private set; }

        /// <summary>
        /// RSA 4096 rather than RSA 1024
        /// </summary>
        public bool IsLargerKey { get; private set; }

        /// <summary>
        /// Get the build information from the required appcasts. Run before accessing the public vars.
        /// </summary>
        /// <param name="input">
        /// The input.
        /// </param>
        public void GetUpdateInfo(string input)
        {
            try
            {
                // Get the correct Appcast and set input.
                XmlNode nodeItem = ReadRss(new XmlTextReader(new StringReader(input)));
                string result = nodeItem.InnerXml;

                // Regular Expressions
                Match ver = Regex.Match(result, @"sparkle:version=""([0-9]*)\""");
                Match verShort = Regex.Match(result, @"sparkle:shortVersionString=""([0-9.]*)");

                this.Build = ver.ToString().Replace("sparkle:version=", string.Empty).Replace("\"", string.Empty);
                if (verShort.Success)
                {
                    this.Version = verShort.Groups[1].Value;
                }
               
                this.DownloadFile = nodeItem["windows"]?.InnerText;
                this.Signature = nodeItem["windowsHash"]?.InnerText;

                string descriptionUrl = nodeItem["sparkle:releaseNotesLink"]?.InnerText;
                if (!string.IsNullOrEmpty(descriptionUrl))
                {
                    this.DescriptionUrl = new Uri(descriptionUrl);
                }

                // We now use a 4096bit key so we need to look for the new key in the appcast.
                // IsLargerKey is a temporary fallback mechanism that we will remove in the following version. 
                string modernSig = nodeItem["windowsSignature"]?.InnerText;
                if (!string.IsNullOrEmpty(modernSig))
                {
                    this.Signature = modernSig;
                    this.IsLargerKey = true;
                }
            }
            catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Read the RSS file.
        /// </summary>
        /// <param name="rssReader">
        /// The RSS reader
        /// </param>
        /// <returns>
        /// The read rss.
        /// </returns>
        private static XmlNode ReadRss(XmlReader rssReader)
        {
            XmlNode nodeItem = null;
            XmlNode nodeChannel = null;
            XmlNode nodeRss = null;

            XmlDocument rssDoc = new XmlDocument();
            rssDoc.Load(rssReader);

            foreach (XmlNode t in rssDoc.ChildNodes)
            {
                if (t.Name == "rss")
                {
                    nodeRss = t;
                }
            }

            if (nodeRss != null)
            {
                foreach (XmlNode t in nodeRss.ChildNodes)
                {
                    if (t.Name == "channel")
                    {
                        nodeChannel = t;
                    }
                }
            }

            if (nodeChannel != null)
            {
                foreach (XmlNode t in nodeChannel.ChildNodes)
                {
                    if (t.Name == "item")
                    {
                        nodeItem = t;
                    }
                }
            }

            return nodeItem;
        }
    }
}