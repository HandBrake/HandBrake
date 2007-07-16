using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Handbrake.Parsing
{
    public class Chapter
    {
        private int m_chapterNumber;
        public int ChapterNumber
        {
            get
            {
                return this.m_chapterNumber;
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

        public override string ToString()
        {
            return this.m_chapterNumber.ToString();
        }

        public static Chapter Parse(StreamReader output)
        {
            string curLine = output.ReadLine();
            if (!curLine.Contains("  + audio tracks:"))
            {
                Chapter thisChapter = new Chapter();
                string[] splitter = curLine.Split(new string[] { "    + ", ": cells ", ", ", " blocks, duration ", "->" }, StringSplitOptions.RemoveEmptyEntries);
                thisChapter.m_chapterNumber = int.Parse(splitter[0]);
                thisChapter.m_duration = TimeSpan.Parse(splitter[4]);
                return thisChapter;
            }
            else
            {
                return null;
            }
        }

        public static Chapter[] ParseList(StreamReader output)
        {
            List<Chapter> chapters = new List<Chapter>();
            string curLine = output.ReadLine();
            while (!curLine.Contains("  + audio tracks:"))
            {
                Chapter thisChapter = Chapter.Parse(output);
           
                if (thisChapter != null)
                {
                    chapters.Add(thisChapter);
                }
                else
                {
                    break;
                }
            }
            return chapters.ToArray();
        }
    }
}
