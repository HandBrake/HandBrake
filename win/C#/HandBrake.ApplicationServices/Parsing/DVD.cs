/*  DVD.cs $    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Parsing
{
    using System.Collections.Generic;
    using System.IO;

    /// <summary>
    /// An object representing a scanned DVD
    /// </summary>
    public class DVD
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DVD"/> class. 
        /// Default constructor for this object
        /// </summary>
        public DVD()
        {
            Titles = new List<Title>();
        }

        /// <summary>
        /// Gets Titles. A list of titles from the source
        /// </summary>
        public List<Title> Titles { get; private set; }

        /// <summary>
        /// Parse the StreamReader output into a List of Titles
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// A DVD object which contains a list of title inforamtion
        /// </returns>
        public static DVD Parse(StreamReader output)
        {
            var thisDVD = new DVD();

            while (!output.EndOfStream)
            {
                if ((char)output.Peek() == '+')
                    thisDVD.Titles.AddRange(Title.ParseList(output.ReadToEnd()));
                else
                    output.ReadLine();
            }

            return thisDVD;
        }
    }
}