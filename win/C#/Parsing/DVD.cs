using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Text.RegularExpressions; 
using System.Windows.Forms;
using System.IO;
using System.Threading;
using System.Diagnostics;

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
            try
            {
                while (!output.EndOfStream)
                {
                    if ((char)output.Peek() == '+')
                    {
                        thisDVD.m_titles.AddRange(Title.ParseList(output.ReadToEnd()));
                    }
                    else
                    {
                        output.ReadLine();
                    }
                }
            }
            catch (Exception exc)
            {
                MessageBox.Show("DVD.CS - Parse" + exc.ToString());
            }
            return thisDVD;
        }
    }
}
