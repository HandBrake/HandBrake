/*  Parser.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.m0k.org/>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace Handbrake.Parsing
{
    /// <summary>
    /// A delegate to handle custom events regarding data being parsed from the buffer
    /// </summary>
    /// <param name="Sender">The object which raised this delegate</param>
    /// <param name="Data">The data parsed from the stream</param>
    public delegate void DataReadEventHandler(object Sender, string Data);

    /// <summary>
    /// A delegate to handle events regarding progress during DVD scanning
    /// </summary>
    /// <param name="Sender">The object who's raising the event</param>
    /// <param name="CurrentTitle">The title number currently being processed</param>
    /// <param name="TitleCount">The total number of titiles to be processed</param>
    public delegate void ScanProgressEventHandler(object Sender, int CurrentTitle, int TitleCount);

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
            try
            {
                
                this.m_buffer += tmp;
                Match m = Regex.Match(tmp, "^Scanning title ([0-9]*) of ([0-9]*)");
                if (OnReadLine != null)
                    OnReadLine(this, tmp);

                if (m.Success && OnScanProgress != null)
                    OnScanProgress(this, int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value));
            }
            catch (Exception exc)
            {
                MessageBox.Show("Parser.cs - ReadLine " + exc.ToString());
            }
            return tmp;
        }

        public override string ReadToEnd()
        {
            string tmp = base.ReadToEnd();
            try
            {
                this.m_buffer += tmp;
                if (OnReadToEnd != null)
                    OnReadToEnd(this, tmp);

            }
            catch (Exception exc)
            {
                MessageBox.Show("Parser.cs - ReadToEnd " + exc.ToString());
            }
            return tmp;
        }
    }
}
