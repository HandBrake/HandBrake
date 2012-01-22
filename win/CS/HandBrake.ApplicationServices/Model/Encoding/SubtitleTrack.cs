// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SubtitleTrack.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Subtitle Information
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model.Encoding
{
    using System;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// Subtitle Information
    /// </summary>
    public class SubtitleTrack : ModelBase
    {
        #region Constants and Fields

        /// <summary>
        /// The source track.
        /// </summary>
        private Subtitle sourceTrack;

        #endregion

        #region Public Properties

        /// <summary>
        ///   Gets or sets a value indicating whether Burned.
        /// </summary>
        public bool Burned { get; set; }

        /// <summary>
        ///   Gets or sets a value indicating whether Default.
        /// </summary>
        public bool Default { get; set; }

        /// <summary>
        ///   Gets or sets a value indicating whether Forced.
        /// </summary>
        public bool Forced { get; set; }

        /// <summary>
        ///   Gets a value indicating whether this is an SRT subtitle.
        /// </summary>
        public bool IsSrtSubtitle
        {
            get
            {
                return this.SrtFileName != "-";
            }
        }

        /// <summary>
        ///   Gets or sets Track.
        /// </summary>
        [Obsolete("Use SourceTrack Instead")]
        public string Track { get; set; }


        /// <summary>
        ///   Gets or sets SourceTrack.
        /// </summary>
        public Subtitle SourceTrack
        {
            get
            {
                return this.sourceTrack;
            }

            set
            {
                this.sourceTrack = value;
                this.OnPropertyChanged("SourceTrack");
                if (this.sourceTrack != null)
                {
                    this.Track = this.sourceTrack.ToString();
                }
            }
        }

        /// <summary>
        ///   Gets or sets the SRT Character Code
        /// </summary>
        public string SrtCharCode { get; set; }

        /// <summary>
        ///   Gets or sets the SRT Filename
        /// </summary>
        public string SrtFileName { get; set; }

        /// <summary>
        ///   Gets or sets the SRT Language
        /// </summary>
        public string SrtLang { get; set; }

        /// <summary>
        ///   Gets or sets the SRT Offset
        /// </summary>
        public int SrtOffset { get; set; }

        /// <summary>
        ///   Gets or sets the Path to the SRT file
        /// </summary>
        public string SrtPath { get; set; }

        /// <summary>
        ///   Gets or sets the type of the subtitle
        /// </summary>
        public SubtitleType SubtitleType { get; set; }

        /// <summary>
        ///   Gets A ListViewItem Containing information about this subitlte
        /// </summary>
        [Obsolete("Used only for the old forms gui. Will be removed.")]
        public ListViewItem ListView
        {
            get
            {
                var listTrack = new ListViewItem(this.Track);
                listTrack.SubItems.Add(this.Forced ? "Yes" : "No");
                listTrack.SubItems.Add(this.Burned ? "Yes" : "No");
                listTrack.SubItems.Add(this.Default ? "Yes" : "No");
                listTrack.SubItems.Add(this.SrtLang);
                listTrack.SubItems.Add(this.SrtCharCode);
                listTrack.SubItems.Add(this.SrtOffset.ToString());
                return listTrack;
            }
        }

        #endregion
    }
}