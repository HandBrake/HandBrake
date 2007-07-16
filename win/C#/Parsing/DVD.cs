using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Windows.Forms;


namespace Handbrake.Parsing
{
    public class DVD
    {
        private List<Title> m_titles;
        public List<Title> Titles
        {
            get
            {
                return this.m_titles;
            }
        }

        public DVD()
        {
            this.m_titles = new List<Title>();
        }

        public static DVD Parse(StreamReader output)
        {
            DVD thisDVD = new DVD();
            while (!output.EndOfStream)
            {
                string curLine = output.ReadLine();

                if (curLine.Contains("Scanning title"))
                {
                    thisDVD.m_titles.AddRange(Title.ParseList(output));
                }
            }
            return thisDVD;
        }
    }
}
