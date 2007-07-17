using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Handbrake.Parsing
{
    public class AudioTrack
    {
        private int m_trackNumber;
        public int TrackNumber
        {
            get
            {
                return this.m_trackNumber;
            }
        }

        private string m_language;
        public string Language
        {
            get
            {
                return this.m_language;
            }
        }

        private string m_format;
        public string Format
        {
            get
            {
                return this.m_format;
            }
        }

        private string m_subFormat;
        public string SubFormat
        {
            get
            {
                return this.m_subFormat;
            }
        }

        private int m_frequency;
        public int Frequency
        {
            get
            {
                return this.m_frequency;
            }
        }

        private int m_bitrate;
        public int Bitrate
        {
            get
            {
                return this.m_bitrate;
            }
        }

        public override string ToString()
        {
            return string.Format("{0} {1} ({2}) ({3})", this.m_trackNumber, this.m_language, this.m_format, this.m_subFormat);
        }

        public static AudioTrack Parse(StreamReader output)
        {
            string curLine = output.ReadLine();
            if (!curLine.Contains("  + subtitle tracks:"))
            {
                AudioTrack thisTrack = new AudioTrack();
                string[] splitter = curLine.Split(new string[] { "    + ", ", ", " (", ") (", " ch", "), ", "Hz, ", "bps" }, StringSplitOptions.RemoveEmptyEntries);
                thisTrack.m_trackNumber = int.Parse(splitter[0]);
                thisTrack.m_language = splitter[1];
                thisTrack.m_format = splitter[2];
                /*
                 * Problem 2
                 * Field 'Handbrake.frmMain.hbProc' is never assigned to, and will always have it's default value null.
                 * This happens with "AllAudios.iso" which is a test file. http://www.sr88.co.uk/AllAudios.iso  (~95MB)
                 */

                thisTrack.m_subFormat = splitter[3];
                thisTrack.m_frequency = int.Parse(splitter[4]);
                thisTrack.m_bitrate = int.Parse(splitter[5]);
                return thisTrack;
            }
            else
            {
                return null;
            }
        }

        public static AudioTrack[] ParseList(StreamReader output)
        {
            List<AudioTrack> tracks = new List<AudioTrack>();
            while (true) // oh glorious hack, serve me well
            {
                AudioTrack thisTrack = AudioTrack.Parse(output);
                if (thisTrack != null)
                {
                    tracks.Add(thisTrack);
                }
                else
                {
                    break;
                }
            }
            return tracks.ToArray();
        }
    }
}
