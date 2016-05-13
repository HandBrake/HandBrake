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
    using System.Runtime.Serialization;
    using System.Xml.Serialization;

    /// <summary>
    /// An object representing a scanned DVD
    /// </summary>
    [DataContract]
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
        [DataMember]
        public string ScanPath { get; set; }

        /// <summary>
        /// Gets or sets Titles. A list of titles from the source
        /// </summary>
        [DataMember]
        [XmlIgnore]
        public List<Title> Titles { get; set; }

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
        }
    }
}