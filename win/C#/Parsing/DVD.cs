using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Windows.Forms;


namespace Handbrake.Parsing
{
    /// <summary>
    /// An object representing a scanned DVD
    /// </summary>
    public class DVD
    {
        private List<Title> m_titles;
        /// <summary>
        /// Collection of Titles associated with this DVD
        /// </summary>
        public List<Title> Titles
        {
            get
            {
                return this.m_titles;
            }
        }

        /// <summary>
        /// Default constructor for this object
        /// </summary>
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
