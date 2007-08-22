using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using System.Text.RegularExpressions;

namespace Handbrake.Parsing
{
    /// <summary>
    /// An object that represents a single Title of a DVD
    /// </summary>
    public class Title
    {
        private List<Chapter> m_chapters;
        /// <summary>
        /// Collection of chapters in this Title
        /// </summary>
        public List<Chapter> Chapters
        {
            get
            {
                return this.m_chapters;
            }
        }

        private List<AudioTrack> m_audioTracks;
        /// <summary>
        /// Collection of audio tracks associated with this Title
        /// </summary>
        public List<AudioTrack> AudioTracks
        {
            get
            {
                return this.m_audioTracks;
            }
        }

        private List<Subtitle> m_subtitles;
        /// <summary>
        /// Collection of subtitles associated with this Title
        /// </summary>
        public List<Subtitle> Subtitles
        {
            get
            {
                return this.m_subtitles;
            }
        }

        private int m_titleNumber;
        /// <summary>
        /// The track number of this Title
        /// </summary>
        public int TitleNumber
        {
            get
            {
                return this.m_titleNumber;
            }
        }

        private TimeSpan m_duration;
        /// <summary>
        /// The length in time of this Title
        /// </summary>
        public TimeSpan Duration
        {
            get
            {
                return this.m_duration;
            }
        }

        private Size m_resolution;
        /// <summary>
        /// The resolution (width/height) of this Title
        /// </summary>
        public Size Resolution
        {
            get
            {
                return this.m_resolution;
            }
        }

        private string m_aspectRatio;
        /// <summary>
        /// The aspect ratio of this Title
        /// </summary>
        public string AspectRatio
        {
            get
            {
                return this.m_aspectRatio;
            }
        }

        private int[] m_autoCrop;
        /// <summary>
        /// The automatically detected crop region for this Title.
        /// This is an int array with 4 items in it as so:
        /// 0: 
        /// 1: 
        /// 2: 
        /// 3: 
        /// </summary>
        public int[] AutoCropDimensions
        {
            get
            {
                return this.m_autoCrop;
            }
        }

        /// <summary>
        /// The constructor for this object
        /// </summary>
        public Title()
        {
            this.m_audioTracks = new List<AudioTrack>();
            this.m_chapters = new List<Chapter>();
            this.m_subtitles = new List<Subtitle>();
        }

        /// <summary>
        /// Override of the ToString method to provide an easy way to use this object in the UI
        /// </summary>
        /// <returns>A string representing this track in the format: {title #} (00:00:00)</returns>
        public override string ToString()
        {
            return string.Format("{0} ({1:00}:{2:00}:{3:00})", this.m_titleNumber, this.m_duration.Hours,
                this.m_duration.Minutes, this.m_duration.Seconds);
        }

        public static Title Parse(StringReader output)
        {
            Title thisTitle = new Title();

            // Match track number for this title
            try
            {

                Match m = Regex.Match(output.ReadLine(), @"^\+ title ([0-9]*):");
                if (m.Success)
                {
                    thisTitle.m_titleNumber = int.Parse(m.Groups[1].Value);
                }
                output.ReadLine();

                // Get duration for this title
                m = Regex.Match(output.ReadLine(), @"^  \+ duration: ([0-9]{2}:[0-9]{2}:[0-9]{2})");
                if (m.Success)
                {
                    thisTitle.m_duration = TimeSpan.Parse(m.Groups[1].Value);
                }

                // Get resolution, aspect ratio and FPS for this title
                m = Regex.Match(output.ReadLine(), @"^  \+ size: ([0-9]*)x([0-9]*), aspect: ([0-9]*\.[0-9]*), ([0-9]*\.[0-9]*) fps");
                if (m.Success)
                {
                    thisTitle.m_resolution = new Size(int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value));
                    thisTitle.m_aspectRatio = m.Groups[3].ToString(); // Converted to a String for French Lanuage based systems. Some weird exception thrown
                                                                      // when trying to parse it as a float
                    // we don't need FPS right now
                }

                // Get autocrop region for this title
                m = Regex.Match(output.ReadLine(), @"^  \+ autocrop: ([0-9]*)/([0-9]*)/([0-9]*)/([0-9]*)");
                if (m.Success)
                {
                    thisTitle.m_autoCrop = new int[4] { int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value), int.Parse(m.Groups[3].Value), int.Parse(m.Groups[4].Value) };
                }

                thisTitle.m_chapters.AddRange(Chapter.ParseList(output));
                thisTitle.m_audioTracks.AddRange(AudioTrack.ParseList(output));
                thisTitle.m_subtitles.AddRange(Subtitle.ParseList(output));
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString());
            }
            return thisTitle;
        }

        public static Title[] ParseList(string output)
        {
            List<Title> titles = new List<Title>();
            StringReader sr = new StringReader(output);
            while ((char)sr.Peek() == '+')
            {
                titles.Add(Title.Parse(sr));
            }
            return titles.ToArray();
        }
    }
}
