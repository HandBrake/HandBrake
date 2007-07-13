using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Handbrake.Parsing
{
    public delegate void DataReadEventHandler(object Sender, string Data);
    /// <summary>
    /// A simple wrapper around a StreamReader to keep track of the entire output from a cli process
    /// </summary>
    internal class Parser : StreamReader
    {
        /// <summary>
        /// The output from the CLI process
        /// </summary>
        private string m_buffer;
        public string Buffer
        {
            get
            {
                return this.m_buffer;
            }
        }

        public static event DataReadEventHandler OnReadLine;
        public static event DataReadEventHandler OnReadToEnd;

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
