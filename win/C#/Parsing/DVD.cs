/*  DVD.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System.Collections.Generic;
using System.IO;

namespace Handbrake.Parsing
{
    /// <summary>
    /// An object representing a scanned DVD
    /// </summary>
    public class DVD
    {
        private readonly List<Title> m_titles;

        /// <summary>
        /// Default constructor for this object
        /// </summary>
        public DVD()
        {
            m_titles = new List<Title>();
        }

        /// <summary>
        /// Collection of Titles associated with this DVD
        /// </summary>
        public List<Title> Titles
        {
            get { return m_titles; }
        }

        public static DVD Parse(StreamReader output)
        {
            var thisDVD = new DVD();

            while (!output.EndOfStream)
            {
                if ((char) output.Peek() == '+')
                    thisDVD.m_titles.AddRange(Title.ParseList(output.ReadToEnd()));
                else
                    output.ReadLine();
            }

            return thisDVD;
        }
    }
}