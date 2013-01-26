// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Parser.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A delegate to handle custom events regarding data being parsed from the buffer
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Parsing
{
    using System;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;

    /// <summary>
    /// A delegate to handle custom events regarding data being parsed from the buffer
    /// </summary>
    /// <param name="sender">The object which raised this delegate</param>
    /// <param name="data">The data parsed from the stream</param>
    public delegate void DataReadEventHandler(object sender, string data);

    /// <summary>
    /// A delegate to handle events regarding progress during DVD scanning
    /// </summary>
    /// <param name="sender">The object who's raising the event</param>
    /// <param name="currentTitle">The title number currently being processed</param>
    /// <param name="titleCount">The total number of titiles to be processed</param>
    public delegate void ScanProgressEventHandler(object sender, int currentTitle, int titleCount);

    /// <summary>
    /// A delegate to handle encode progress updates // EXPERIMENTAL
    /// </summary>
    /// <param name="sender">The object which raised the event</param>
    /// <param name="currentTask">The current task being processed from the queue</param>
    /// <param name="taskCount">The total number of tasks in queue</param>
    /// <param name="percentComplete">The percentage this task is complete</param>
    /// <param name="currentFps">The current encoding fps</param>
    /// <param name="averageFps">The average encoding fps for this task</param>
    /// <param name="timeRemaining">The estimated time remaining for this task to complete</param>
    public delegate void EncodeProgressEventHandler(object sender, int currentTask, int taskCount, float percentComplete, float currentFps, float averageFps, string timeRemaining);

    /// <summary>
    /// A simple wrapper around a StreamReader to keep track of the entire output from a cli process
    /// </summary>
    public class Parser : StreamReader
    {
        /// <summary>
        /// The Buffer StringBuilder
        /// </summary>
        private readonly StringBuilder buffer = new StringBuilder(string.Empty);

        /// <summary>
        /// Initializes a new instance of the <see cref="Parser"/> class. 
        /// Default constructor for this object
        /// </summary>
        /// <param name="baseStream">
        /// The stream to parse from
        /// </param>
        public Parser(Stream baseStream) : base(baseStream, Encoding.Default)
        {
        }

        /// <summary>
        /// Raised upon a new line being read from stdout/stderr
        /// </summary>
        public event DataReadEventHandler OnReadLine;

        /// <summary>
        /// Raised upon the entire stdout/stderr stream being read in a single call
        /// </summary>
        public event DataReadEventHandler OnReadToEnd;

        /// <summary>
        /// Raised upon the catching of a "Scanning title # of #..." in the stream
        /// </summary>
        public event ScanProgressEventHandler OnScanProgress;


        /// <summary>
        /// Gets the buffer of data that came from the CLI standard input/error
        /// </summary>
        public StringBuilder Buffer
        {
            get { return buffer; }
        }

        /// <summary>
        /// Read a line from standard in/err
        /// </summary>
        /// <returns>
        /// The read line
        /// </returns>
        public override string ReadLine()
        {
            string tmp = base.ReadLine();

            buffer.Append(tmp + Environment.NewLine);

            Match m = null;
            if (tmp.Contains("Scanning title"))
                m = Regex.Match(tmp, "^Scanning title ([0-9]*) of ([0-9]*)");

            if (OnReadLine != null)
                OnReadLine(this, tmp);

            if (m != null)
                if (m.Success && OnScanProgress != null)
                    OnScanProgress(this, int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value));

            return tmp;
        }

        /// <summary>
        /// Read to the end of the input stream
        /// </summary>
        /// <returns>
        /// A string of the input data
        /// </returns>
        public override string ReadToEnd()
        {
            string tmp = base.ReadToEnd();

            buffer.Append(tmp + Environment.NewLine);
            if (OnReadToEnd != null)
                OnReadToEnd(this, tmp);

            return tmp;
        }
    }
}