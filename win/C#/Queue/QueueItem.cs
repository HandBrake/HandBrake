namespace Handbrake.Queue
{
    public class QueueItem
    {
        /// <summary>
        /// Get or Set the job id.
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// Get or Set the query string.
        /// </summary>
        public string Query { get; set; }

        /// <summary>
        /// Get or set the source file of encoding
        /// </summary>
        public string Source { get; set; }

        /// <summary>
        /// Get or set the destination for the file to be encoded.
        /// </summary>
        public string Destination { get; set; }
    }
}