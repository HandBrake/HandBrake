/*  Parser.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Parsing
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
    public delegate void EncodeProgressEventHandler(object sender, int currentTask, int taskCount, float percentComplete, float currentFps, float averageFps, TimeSpan timeRemaining);

    /// <summary>
    /// A simple wrapper around a StreamReader to keep track of the entire output from a cli process
    /// </summary>
    internal class Parser : StreamReader
    {
        /// <summary>
        /// The Buffer StringBuilder
        /// </summary>
        private StringBuilder buffer = new StringBuilder(string.Empty);

        /// <summary>
        /// Initializes a new instance of the <see cref="Parser"/> class. 
        /// Default constructor for this object
        /// </summary>
        /// <param name="baseStream">
        /// The stream to parse from
        /// </param>
        public Parser(Stream baseStream) : base(baseStream)
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
        /// Raised upon the catching of a "Scanning title # of #..." in the stream
        /// </summary>
        public event EncodeProgressEventHandler OnEncodeProgress;

        /// <summary>
        /// Gets the buffer of data that came from the CLI standard input/error
        /// </summary>
        public string Buffer
        {
            get { return buffer.ToString(); }
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

        /// <summary>
        /// Pase the CLI status output (from standard output)
        /// </summary>
        public void ReadEncodeStatus()
        {
            CultureInfo culture = CultureInfo.CreateSpecificCulture("en-US");
            string tmp = base.ReadLine();

            Match m = Regex.Match(tmp, @"^Encoding: task ([0-9]*) of ([0-9]*), ([0-9]*\.[0-9]*) %( \(([0-9]*\.[0-9]*) fps, avg ([0-9]*\.[0-9]*) fps, ETA ([0-9]{2})h([0-9]{2})m([0-9]{2})s\))?");
            if (m.Success && OnEncodeProgress != null)
            {
                int currentTask = int.Parse(m.Groups[1].Value);
                int totalTasks = int.Parse(m.Groups[2].Value);
                float percent = float.Parse(m.Groups[3].Value, culture);
                float currentFps = m.Groups[5].Value == string.Empty ? 0.0F : float.Parse(m.Groups[5].Value, culture);
                float avgFps = m.Groups[6].Value == string.Empty ? 0.0F : float.Parse(m.Groups[6].Value, culture);
                TimeSpan remaining = TimeSpan.Zero;
                if (m.Groups[7].Value != string.Empty)
                {
                    remaining = TimeSpan.Parse(m.Groups[7].Value + ":" + m.Groups[8].Value + ":" + m.Groups[9].Value);
                }
                OnEncodeProgress(this, currentTask, totalTasks, percent, currentFps, avgFps, remaining);
            }
        }
    }
}