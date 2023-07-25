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
    using System.Xml.Serialization;

    using HandBrake.App.Core.Model;
    using HandBrake.App.Core.Utilities;

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

        public Source(Source scannedSource) : this(scannedSource.Titles)
        { 
        }

        public Source(List<Title> titles)
        {
            this.Titles = titles;
        }

        /// <summary>
        /// Gets or sets Titles. A list of titles from the source
        /// </summary>
        [XmlIgnore]
        public List<Title> Titles { get; }
    }
}