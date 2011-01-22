/*  QueueTask.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model
{
    /// <summary>
    /// The QueueTask.
    /// </summary>
    public class QueueTask
    {
        /*
         * TODO
         * - Update the Query property to generate the query from the EncodeTask object.
         * - Remove Sourcee, Destination and Title when they are no longer used.
         */

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueTask"/> class.
        /// </summary>
        public QueueTask()
        {         
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="QueueTask"/> class.
        /// </summary>
        /// <param name="query">
        /// The query.
        /// </param>
        public QueueTask(string query)
        {
            this.Query = query;
        }

        /// <summary>
        /// Gets or sets the job ID.
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets Title.
        /// </summary>
        public int Title { get; set; }

        /// <summary>
        /// Gets or sets Source.
        /// </summary>
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets Destination.
        /// </summary>
        public string Destination { get; set; }

        /// <summary>
        /// Gets or sets the query string.
        /// </summary>
        public string Query { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether if this is a user or GUI generated query
        /// </summary>
        public bool CustomQuery { get; set; }

        /// <summary>
        /// Gets or sets the Encode Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        /// <summary>
        /// Gets a value indicating whether or not this instance is empty.
        /// </summary>
        public bool IsEmpty
        {
            get
            {
                return this.Id == 0 && string.IsNullOrEmpty(this.Query) && string.IsNullOrEmpty(this.Task.Source) &&
                       string.IsNullOrEmpty(this.Task.Destination);
            }
        }
    }
}