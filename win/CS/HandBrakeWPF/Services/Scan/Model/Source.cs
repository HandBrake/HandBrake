// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Source.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object representing a scanned DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Model
{
    using System.Collections.Generic;
    using System.IO;
    using System.Xaml;
    using System.Xml.Serialization;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Utilities;

    /// <summary>
    /// An object representing a scanned DVD
    /// </summary>
    public class Source
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Source"/> class. 
        /// Default constructor for this object
        /// </summary>
        public Source()
        {
            this.Titles = new List<Title>();
        }

        /// <summary>
        /// Gets or sets ScanPath.
        /// The Path used by the Scan Service.
        /// </summary>
        public string ScanPath { get; set; }

        /// <summary>
        /// Gets or sets Titles. A list of titles from the source
        /// </summary>
        [XmlIgnore]
        public List<Title> Titles { get; set; }

        public string SourceName { get; private set; }

        /// <summary>
        /// Copy this Source to another Source Model
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        public void CopyTo(Source source)
        {
            source.Titles = this.Titles;
            source.ScanPath = this.ScanPath;

            // Scan Path is a File.
            if (File.Exists(this.ScanPath))
            {
                this.SourceName = Path.GetFileNameWithoutExtension(this.ScanPath);
            }

            // Scan Path is a folder.
            if (Directory.Exists(this.ScanPath))
            {
                // Check to see if it's a Drive. If yes, use the volume label.
                foreach (DriveInformation item in DriveUtilities.GetDrives())
                {
                    if (item.RootDirectory.Contains(this.ScanPath.Replace("\\\\", "\\")))
                    {
                        this.SourceName = item.VolumeLabel;
                    }
                }

                // Otherwise, it may be a path of files.
                if (string.IsNullOrEmpty(this.SourceName))
                {
                    this.SourceName = Path.GetFileName(this.ScanPath);
                }
            }
        }
    }
}