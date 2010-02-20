/*  Subtitle.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Model
{
    using System.Windows.Forms;

    /// <summary>
    /// Subtitle Information
    /// </summary>
    public class SubtitleInfo
    {
        /// <summary>
        /// Gets or sets the Subtitle Track
        /// </summary>
        public string Track { get; set; }

        /// <summary>
        /// Gets or sets the Forced Subtitle
        /// </summary>
        public string Forced { get; set; }

        /// <summary>
        /// Gets or sets the Burned In Subtitle
        /// </summary>
        public string Burned { get; set; }

        /// <summary>
        /// Gets or sets the Default Subtitle Track
        /// </summary>
        public string Default { get; set; }

        /// <summary>
        /// Gets or sets the SRT Language
        /// </summary>
        public string SrtLang { get; set; }

        /// <summary>
        /// Gets or sets the SRT Character Code
        /// </summary>
        public string SrtCharCode { get; set; }

        /// <summary>
        /// Gets or sets the SRT Offset
        /// </summary>
        public int SrtOffset { get; set; }

        /// <summary>
        /// Gets or sets the Path to the SRT file
        /// </summary>
        public string SrtPath { get; set; }

        /// <summary>
        /// Gets or sets the SRT Filename
        /// </summary>
        public string SrtFileName { get; set; }

        /// <summary>
        /// Gets a value indicating whether this is an SRT subtitle.
        /// </summary>
        public bool IsSrtSubtitle
        {
            get { return this.SrtFileName != "-"; }
        }

        /// <summary>
        /// Gets A ListViewItem Containing information about this subitlte
        /// </summary>
        public ListViewItem ListView
        {
            get
            {
                var listTrack = new ListViewItem(this.Track);
                listTrack.SubItems.Add(this.Forced);
                listTrack.SubItems.Add(this.Burned);
                listTrack.SubItems.Add(this.Default);
                listTrack.SubItems.Add(this.SrtLang);
                listTrack.SubItems.Add(this.SrtCharCode);
                listTrack.SubItems.Add(this.SrtOffset.ToString());
                return listTrack;
            }
        }
    }
}