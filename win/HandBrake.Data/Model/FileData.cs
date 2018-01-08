// --------------------------------------------------------------------------------------------------------------------
// <copyright file="FileData.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Model
{
    using System.Runtime.Serialization;
    using System.Xml.Serialization;
    using PlatformBindings;
    using PlatformBindings.Models.FileSystem;

    [DataContract]
    public class FileData
    {
        [XmlIgnore]
        private FileContainer file;

        /// <summary>
        /// Initializes a new instance of the <see cref="FileData"/> class.
        /// </summary>
        public FileData()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FileData"/> class.
        /// </summary>
        /// <param name="file">
        /// File instance.
        /// </param>
        public FileData(FileContainer file)
        {
            this.File = file;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FileData"/> class.
        /// </summary>
        /// <param name="path">
        /// Path instance.
        /// </param>
        public FileData(string path)
        {
            this.Path = path;
        }

        /// <summary>
        /// Gets or sets the File Instance.
        /// </summary>
        [XmlIgnore]
        public FileContainer File
        {
            get
            {
                if (this.file != null)
                {
                    return this.file;
                }
                else if (!string.IsNullOrWhiteSpace(this.Path))
                {
                    this.file = AppServices.Current?.IO?.GetFile(this.Path)?.Result;
                }

                return null;
            }

            set
            {
                this.file = value;
                this.Path = value.Path;
            }
        }

        /// <summary>
        /// Gets or sets the Path of the File.
        /// </summary>
        public string Path { get; set; }
    }
}