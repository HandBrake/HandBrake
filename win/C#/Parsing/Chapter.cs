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

        /*private int[] m_cellRange;
        public int[] CellRange
        {
            get
            {
                return this.m_cellRange;
            }
        }

        private int m_blocks;
        public int BlockCount
        {
            get
            {
                return this.m_blocks;
            }
        }*/

        private TimeSpan m_duration;
        public TimeSpan Duration
        {
            get
            {
                return this.m_duration;
            }
        }

        public static Chapter Parse(StreamReader output)
        {
            string curLine = output.ReadLine();
            if (!curLine.Contains("  + audio tracks:"))
            {
                Chapter thisChapter = new Chapter();
                string[] splitter = curLine.Split(new string[] { "    + ", ": cells ", ", ", " blocks, duration ", "->" }, StringSplitOptions.RemoveEmptyEntries);
                thisChapter.m_chapterNumber = int.Parse(splitter[0]);
                //thisChapter.m_cellRange = new int[2] { int.Parse(splitter[1]), int.Parse(splitter[2]) };
                //thisChapter.m_blocks = int.Parse(splitter[3]);
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
