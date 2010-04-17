/*  DVD.cs $
 	
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Parsing
{
    using System.Collections.Generic;
    using System.IO;

    /// <summary>
    /// An object representing a scanned DVD
    /// </summary>
    public class DVD
    {
        private readonly List<Title> titles;

        /// <summary>
        /// Initializes a new instance of the <see cref="DVD"/> class. 
        /// Default constructor for this object
        /// </summary>
        public DVD()
        {
            titles = new List<Title>();
        }

        /// <summary>
        /// Collection of Titles associated with this DVD
        /// </summary>
        public List<Title> Titles
        {
            get { return titles; }
        }

        public static DVD Parse(StreamReader output)
        {
            var thisDVD = new DVD();

            while (!output.EndOfStream)
            {
                if ((char) output.Peek() == '+')
                    thisDVD.titles.AddRange(Title.ParseList(output.ReadToEnd()));
                else
                    output.ReadLine();
            }

            return thisDVD;
        }
    }
}