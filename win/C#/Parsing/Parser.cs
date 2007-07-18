using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Handbrake.Parsing
{
    /// <summary>
    /// A delegate to handle custom events regarding data being parsed from the buffer
    /// </summary>
    /// <param name="Sender">The object which raised this delegate</param>
    /// <param name="Data">The data parsed from the stream</param>
    public delegate void DataReadEventHandler(object Sender, string Data);

    /// <summary>
    /// A simple wrapper around a StreamReader to keep track of the entire output from a cli process
    /// </summary>
    internal class Parser : StreamReader
    {
        private string m_buffer;
        /// <summary>
        /// The output from the CLI process
        /// </summary>
        public string Buffer
        {
            get
            {
                return this.m_buffer;
            }
        }

        /// <summary>
        /// Raised upon a new line being read from stdout/stderr
        /// </summary>
        public static event DataReadEventHandler OnReadLine;

        /// <summary>
        /// Raised upon the entire stdout/stderr stream being read in a single call
        /// </summary>
        public static event DataReadEventHandler OnReadToEnd;

        /// <summary>
        /// Default constructor for this object
        /// </summary>
        /// <param name="baseStream">The stream to parse from</param>
        public Parser(Stream baseStream) : base(baseStream)
        {
            this.m_buffer = string.Empty;
        }

        public override string ReadLine()
        {
            string tmp = base.ReadLine();
            this.m_buffer += tmp;
            if (OnReadLine != null)
            {
                OnReadLine(this, tmp);
            }
            return tmp;
        }

        public override string ReadToEnd()
        {
            string tmp = base.ReadToEnd();
            this.m_buffer += tmp;
            if (OnReadToEnd != null)
            {
                OnReadToEnd(this, tmp);
            }
            return tmp;
        }
    }
}
