/*  QueueItem.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

namespace Handbrake.EncodeQueue
{
    public struct Job
    {
        /// <summary>
        /// Gets or sets the job ID.
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets the query string.
        /// </summary>
        public string Query { get; set; }

        /// <summary>
        /// Gets or sets the source file of encoding.
        /// </summary>
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets the destination for the file to be encoded.
        /// </summary>
        public string Destination { get; set; }

        /// <summary>
        /// Gets whether or not this instance is empty.
        /// </summary>
        public bool IsEmpty
        {
            get { return Id == 0 && string.IsNullOrEmpty(Query) && string.IsNullOrEmpty(Source) && string.IsNullOrEmpty(Destination); }
        }
    }
}