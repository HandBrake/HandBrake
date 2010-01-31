/*  Subtitle.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */
using System.Windows.Forms;

namespace Handbrake.Model
{
    /// <summary>
    /// Subtitle Information
    /// </summary>
    public class SubtitleInfo
    {
        /// <summary>
        /// Subtitle Track
        /// </summary>
        public string Track { get; set; }

        /// <summary>
        /// Forced Subtitle
        /// </summary>
        public string Forced { get; set; }

        /// <summary>
        /// Burned In Subtitle
        /// </summary>
        public string Burned { get; set; }

        /// <summary>
        /// Default Subtitle Track
        /// </summary>
        public string Default { get; set; }

        /// <summary>
        /// SRT Language
        /// </summary>
        public string SrtLang { get; set; }

        /// <summary>
        /// SRT Character Code
        /// </summary>
        public string SrtCharCode { get; set; }

        /// <summary>
        /// SRT Offset
        /// </summary>
        public int SrtOffset { get; set; }

        /// <summary>
        /// Path to the SRT file
        /// </summary>
        public string SrtPath { get; set; }

        /// <summary>
        /// SRT Filename
        /// </summary>
        public string SrtFileName { get; set; }

        /// <summary>
        /// A ListViewItem Containing information about this subitlte
        /// </summary>
        public ListViewItem ListView
        {
            get
            {
                ListViewItem listTrack = new ListViewItem(Track);
                listTrack.SubItems.Add(Forced);
                listTrack.SubItems.Add(Burned);
                listTrack.SubItems.Add(Default);
                listTrack.SubItems.Add(SrtLang);
                listTrack.SubItems.Add(SrtCharCode);
                listTrack.SubItems.Add(SrtOffset.ToString());
                return listTrack;
            }
        }
    }
}
