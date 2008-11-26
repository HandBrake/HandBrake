using System;
using System.Collections.Generic;
using System.Text;

namespace Handbrake.Queue
{
    public class QueueItem
    {
        private int id;
        private string query;
        private string source;
        private string destination;

        /// <summary>
        /// Get or Set the job id.
        /// </summary>
        public int Id
        {
            get { return id; }
            set { this.id = value; }
        }

        /// <summary>
        /// Get or Set the query string.
        /// </summary>
        public string Query
        {
            get { return query; }
            set { this.query = value; }
        }

        /// <summary>
        /// Get or set the source file of encoding
        /// </summary>
        public string Source
        {
            get { return source; }
            set { this.source = value; }
        }

        /// <summary>
        /// Get or set the destination for the file to be encoded.
        /// </summary>
        public string Destination
        {
            get { return destination; }
            set { this.destination = value; }
        }
    }
}
