using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace Handbrake.Parsing
{
    public class Title
    {
        private List<Chapter> m_chapters;
        public List<Chapter> Chapters
        {
            get
            {
                return this.m_chapters;
            }
        }

        private List<AudioTrack> m_audioTracks;
        public List<AudioTrack> AudioTracks
        {
            get
            {
                return this.m_audioTracks;
            }
        }

        private List<Subtitle> m_subtitles;
        public List<Subtitle> Subtitles
        {
            get
            {
                return this.m_subtitles;
            }
        }

        private int m_titleNumber;
        public int TitleNumber
        {
            get
            {
                return this.m_titleNumber;
            }
        }

        private TimeSpan m_duration;
        public TimeSpan Duration
        {
            get
            {
                return this.m_duration;
            }
        }

        private Size m_resolution;
        public Size Resolution
        {
            get
            {
                return this.m_resolution;
            }
        }

        private float m_aspectRatio;
        public float AspectRatio
        {
            get
            {
                return this.m_aspectRatio;
            }
        }


        private int[] m_autoCrop;
        public int[] AutoCropDimensions
        {
            get
            {
                return this.m_autoCrop;
            }
        }

        public Title()
        {
            this.m_audioTracks = new List<AudioTrack>();
            this.m_chapters = new List<Chapter>();
            this.m_subtitles = new List<Subtitle>();
        }

        public override string ToString()
        {
            return string.Format("{0} ({1:00}:{2:00}:{3:00})", this.m_titleNumber, this.m_duration.Hours, 
                this.m_duration.Minutes, this.m_duration.Seconds);
        }

        public static Title Parse(StreamReader output)
        {
            Title thisTitle = new Title();

            /*
             * This will be converted to use Regex soon, I promise ;)
             * brianmario - 7/9/07
             */
        
                string curLine = output.ReadLine();
                thisTitle.m_titleNumber = int.Parse(curLine.Substring(curLine.Length - 2, 1));
                curLine = output.ReadLine();
                string[] splitter = curLine.Split(',');
   
                splitter = splitter[2].Trim().Split(' ', '(', ')');

                splitter = splitter[1].Split('-', '>');

                curLine = output.ReadLine();
                splitter = curLine.Split(new string[] { "  + duration: " }, StringSplitOptions.RemoveEmptyEntries);
                thisTitle.m_duration = TimeSpan.Parse(splitter[0]);
                curLine = output.ReadLine();
                splitter = curLine.Split(new string[] { "  + size: ", "aspect: ", ", ", " fps", "x" }, StringSplitOptions.RemoveEmptyEntries);
                thisTitle.m_resolution = new Size(int.Parse(splitter[0]), int.Parse(splitter[1]));
                thisTitle.m_aspectRatio = float.Parse(splitter[2].ToString());
     
                curLine = output.ReadLine();
                splitter = curLine.Split(new string[] { "  + autocrop: ", "/" }, StringSplitOptions.RemoveEmptyEntries);
                thisTitle.m_autoCrop = new int[4] { int.Parse(splitter[0]), int.Parse(splitter[1]), int.Parse(splitter[2]), int.Parse(splitter[3]) };
                thisTitle.m_chapters.AddRange(Chapter.ParseList(output));
                thisTitle.m_audioTracks.AddRange(AudioTrack.ParseList(output));
                thisTitle.m_subtitles.AddRange(Subtitle.ParseList(output));
            
            return thisTitle;
        }

        public static Title[] ParseList(StreamReader output)
        {
            List<Title> titles = new List<Title>();
            while ((char)output.Peek() == '+')
            {
                titles.Add(Title.Parse(output));
            }
            return titles.ToArray();
        }
    }
}
