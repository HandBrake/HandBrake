/*  Title.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Parsing
{
    using System;
    using System.Collections.Generic;
    using System.Drawing;
    using System.Globalization;
    using System.IO;
    using System.Text.RegularExpressions;

    /// <summary>
    /// An object that represents a single Title of a DVD
    /// </summary>
    public class Title
    {
        /// <summary>
        /// The Culture Info
        /// </summary>
        private static readonly CultureInfo Culture = new CultureInfo("en-US", false);

        /// <summary>
        /// Initializes a new instance of the <see cref="Title"/> class. 
        /// The constructor for this object
        /// </summary>
        public Title()
        {
            Angles = new List<string>();
            AudioTracks = new List<AudioTrack>();
            Chapters = new List<Chapter>();
            Subtitles = new List<Subtitle>();
        }

        /// <summary>
        /// Gets a Collection of chapters in this Title
        /// </summary>
        public List<Chapter> Chapters { get; private set; }

        /// <summary>
        /// Gets a Collection of audio tracks associated with this Title
        /// </summary>
        public List<AudioTrack> AudioTracks { get; private set; }

        /// <summary>
        /// Gets aCollection of subtitles associated with this Title
        /// </summary>
        public List<Subtitle> Subtitles { get; private set; }

        /// <summary>
        /// Gets The track number of this Title
        /// </summary>
        public int TitleNumber { get; private set; }

        /// <summary>
        /// Gets a value indicating whether this is a MainTitle.
        /// </summary>
        public bool MainTitle { get; private set; }

        /// <summary>
        /// Gets the Source Name
        /// </summary>
        public string SourceName { get; private set; }

        /// <summary>
        /// Gets the length in time of this Title
        /// </summary>
        public TimeSpan Duration { get; private set; }

        /// <summary>
        /// Gets the resolution (width/height) of this Title
        /// </summary>
        public Size Resolution { get; private set; }

        /// <summary>
        /// Gets the aspect ratio of this Title
        /// </summary>
        public float AspectRatio { get; private set; }

        /// <summary>
        /// Gets Par Value
        /// </summary>
        public Size ParVal { get; private set; }

        /// <summary>
        /// Gets the automatically detected crop region for this Title.
        /// This is an int array with 4 items in it as so:
        /// 0: T
        /// 1: B
        /// 2: L
        /// 3: R
        /// </summary>
        public int[] AutoCropDimensions { get; private set; }

        /// <summary>
        /// Gets a Collection of Angles in this Title
        /// </summary>
        public List<string> Angles { get; private set; }

        /// <summary>
        /// Gets the FPS of the source.
        /// </summary>
        public float Fps { get; private set; }

        /// <summary>
        /// Parse the Title Information
        /// </summary>
        /// <param name="output">A stingreader of output data</param>
        /// <returns>A Title</returns>
        public static Title Parse(StringReader output)
        {
            var thisTitle = new Title();

            Match m = Regex.Match(output.ReadLine(), @"^\+ title ([0-9]*):");
            // Match track number for this title
            if (m.Success)
                thisTitle.TitleNumber = int.Parse(m.Groups[1].Value.Trim());

            // If we are scanning a groupd of files, we'll want to get the source name.
            string path = output.ReadLine();

            m = Regex.Match(path, @"  \+ Main Feature");
            if (m.Success)
            {
                thisTitle.MainTitle = true;
                path = output.ReadLine();
            }

            m = Regex.Match(path, @"^  \+ stream:");
            if (m.Success)
                thisTitle.SourceName = path.Replace("+ stream:", string.Empty).Trim();

            if (!Properties.Settings.Default.noDvdNav)
            {
                // Get the Angles for the title.
                m = Regex.Match(output.ReadLine(), @"  \+ angle\(s\) ([0-9])");
                if (m.Success)
                {
                    string angleList = m.Value.Replace("+ angle(s) ", string.Empty).Trim();
                    int angleCount;
                    int.TryParse(angleList, out angleCount);

                    for (int i = 1; i <= angleCount; i++)
                        thisTitle.Angles.Add(i.ToString());
                }
            }

            // Get duration for this title
            m = Regex.Match(output.ReadLine(), @"^  \+ duration: ([0-9]{2}:[0-9]{2}:[0-9]{2})");
            if (m.Success)
                thisTitle.Duration = TimeSpan.Parse(m.Groups[1].Value);

            // Get resolution, aspect ratio and FPS for this title
            m = Regex.Match(output.ReadLine(), @"^  \+ size: ([0-9]*)x([0-9]*), pixel aspect: ([0-9]*)/([0-9]*), display aspect: ([0-9]*\.[0-9]*), ([0-9]*\.[0-9]*) fps");
            if (m.Success)
            {
                thisTitle.Resolution = new Size(int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value));
                thisTitle.ParVal = new Size(int.Parse(m.Groups[3].Value), int.Parse(m.Groups[4].Value));
                thisTitle.AspectRatio = float.Parse(m.Groups[5].Value, Culture);
                thisTitle.Fps = float.Parse(m.Groups[6].Value, Culture);
            }

            // Get autocrop region for this title
            m = Regex.Match(output.ReadLine(), @"^  \+ autocrop: ([0-9]*)/([0-9]*)/([0-9]*)/([0-9]*)");
            if (m.Success)
                thisTitle.AutoCropDimensions = new[]
                                           {
                                               int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value), 
                                               int.Parse(m.Groups[3].Value), int.Parse(m.Groups[4].Value)
                                           };

            thisTitle.Chapters.AddRange(Chapter.ParseList(output));

            thisTitle.AudioTracks.AddRange(AudioTrack.ParseList(output));

            thisTitle.Subtitles.AddRange(Subtitle.ParseList(output));

            return thisTitle;
        }

        /// <summary>
        /// Return a list of parsed titles
        /// </summary>
        /// <param name="output">The Output</param>
        /// <returns>A List of titles</returns>
        public static Title[] ParseList(string output)
        {
            var titles = new List<Title>();
            var sr = new StringReader(output);

            while (sr.Peek() == '+' || sr.Peek() == ' ')
            {
                // If the the character is a space, then chances are the line
                if (sr.Peek() == ' ') // If the character is a space, then chances are it's the combing detected line.
                    sr.ReadLine(); // Skip over it
                else
                    titles.Add(Parse(sr));
            }

            return titles.ToArray();
        }

        /// <summary>
        /// Override of the ToString method to provide an easy way to use this object in the UI
        /// </summary>
        /// <returns>A string representing this track in the format: {title #} (00:00:00)</returns>
        public override string ToString()
        {
            return string.Format("{0} ({1:00}:{2:00}:{3:00})", TitleNumber, Duration.Hours, Duration.Minutes, Duration.Seconds);
        }
    }
}