/*  QueueItem.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model
{
    /// <summary>
    /// The job.
    /// </summary>
    public struct Job
    {
        /// <summary>
        /// Gets or sets the job ID.
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets the selected Title.
        /// </summary>
        public int Title { get; set; }

        /// <summary>
        /// Gets or sets the query string.
        /// </summary>
        public string Query { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether if this is a user or GUI generated query
        /// </summary>
        public bool CustomQuery { get; set; }

        /// <summary>
        /// Gets or sets the source file of encoding.
        /// </summary>
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets the destination for the file to be encoded.
        /// </summary>
        public string Destination { get; set; }

        /// <summary>
        /// Gets a value indicating whether or not this instance is empty.
        /// </summary>
        public bool IsEmpty
        {
            get
            {
                return this.Id == 0 && string.IsNullOrEmpty(this.Query) && string.IsNullOrEmpty(this.Source) &&
                       string.IsNullOrEmpty(this.Destination);
            }
        }
    }
}