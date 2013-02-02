// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Title.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An object that represents a single Title of a DVD
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Parsing
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text.RegularExpressions;

    using HandBrake.Interop.Model;

    using Size = System.Drawing.Size;

    /// <summary>
    /// An object that represents a single Title of a DVD
    /// </summary>
    public class Title
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Title"/> class. 
        /// </summary>
        public Title()
        {
            this.AudioTracks = new List<Audio>();
            this.Chapters = new List<Chapter>();
            this.Subtitles = new List<Subtitle>();
        }

        #region Properties

        /// <summary>
        /// Gets or sets a Collection of chapters in this Title
        /// </summary>
        public List<Chapter> Chapters { get; set; }

        /// <summary>
        /// Gets or sets a Collection of audio tracks associated with this Title
        /// </summary>
        public List<Audio> AudioTracks { get; set; }

        /// <summary>
        /// Gets or sets a Collection of subtitles associated with this Title
        /// </summary>
        public List<Subtitle> Subtitles { get; set; }

        /// <summary>
        /// Gets or sets The track number of this Title
        /// </summary>
        public int TitleNumber { get; set; }

        /// <summary>
        /// Gets or sets Playlist.
        /// </summary>
        public string Playlist { get; set; }

        /// <summary>
        /// Gets or sets the length in time of this Title
        /// </summary>
        public TimeSpan Duration { get; set; }

        /// <summary>
        /// Gets or sets the resolution (width/height) of this Title
        /// </summary>
        public Size Resolution { get; set; }

        /// <summary>
        /// Gets or sets the aspect ratio of this Title
        /// </summary>
        public double AspectRatio { get; set; }

        /// <summary>
        /// Gets or sets AngleCount.
        /// </summary>
        public int AngleCount { get; set; }

        /// <summary>
        /// Gets or sets Par Value
        /// </summary>
        public Size ParVal { get; set; }

        /// <summary>
        /// Gets or sets the automatically detected crop region for this Title.
        /// This is an int array with 4 items in it as so:
        /// 0: T
        /// 1: B
        /// 2: L
        /// 3: R
        /// </summary>
        public Cropping AutoCropDimensions { get; set; }

        /// <summary>
        /// Gets or sets the FPS of the source.
        /// </summary>
        public double Fps { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this is a MainTitle.
        /// </summary>
        public bool MainTitle { get; set; }

        /// <summary>
        /// Gets or sets the Source Name
        /// </summary>
        public string SourceName { get; set; }

        /// <summary>
        /// Gets or sets the OpenCL
        /// </summary>
        public int OpenCLSupport { get; set; }

        /// <summary>
        /// Gets or sets the HWD
        /// </summary>
        public int HWDSupport { get; set; }
        #endregion

        /// <summary>
        /// Parse the Title Information
        /// </summary>
        /// <param name="output">
        /// A StringReader of output data
        /// </param>
        /// <param name="isDvdNavDisabled">
        /// The is Dvd Nav Disabled.
        /// </param>
        /// <returns>
        /// A Title Object
        /// </returns>
        public static Title Parse(StringReader output, bool isDvdNavDisabled)
        {
            var thisTitle = new Title();
            string nextLine = output.ReadLine();
            string temp;

            // Get the Title Number
            Match m = Regex.Match(nextLine, @"^\+ title ([0-9]*):");
            if (m.Success)
                thisTitle.TitleNumber = int.Parse(m.Groups[1].Value.Trim());
            nextLine = output.ReadLine();

            // Detect if this is the main feature
            m = Regex.Match(nextLine, @"  \+ Main Feature");
            if (m.Success)
            {
                thisTitle.MainTitle = true;
                nextLine = output.ReadLine();
            }

            // Get the stream name for file import
            m = Regex.Match(nextLine, @"^  \+ stream:");
            if (m.Success)
            {
                thisTitle.SourceName = nextLine.Replace("+ stream:", string.Empty).Trim();
                nextLine = output.ReadLine();
            }

            // Playlist
            m = Regex.Match(nextLine, @"^  \+ playlist:");
            if (m.Success)
            {
                thisTitle.Playlist = nextLine.Replace("+ playlist:", string.Empty).Trim();
                nextLine = output.ReadLine();
            }

            // Jump over the VTS and blocks line
            m = Regex.Match(nextLine, @"^  \+ vts:");
            if (nextLine.Contains("blocks") || nextLine.Contains("+ vts "))
            {
                nextLine = output.ReadLine();
            }
       
            // Multi-Angle Support if LibDvdNav is enabled
            if (!isDvdNavDisabled)
            {
                m = Regex.Match(nextLine, @"  \+ angle\(s\) ([0-9])");
                if (m.Success)
                {
                    string angleList = m.Value.Replace("+ angle(s) ", string.Empty).Trim();
                    int angleCount;
                    int.TryParse(angleList, out angleCount);

                    thisTitle.AngleCount = angleCount;
                    nextLine = output.ReadLine();
                }
            }

            // Get duration for this title
            m = Regex.Match(nextLine, @"^  \+ duration: ([0-9]{2}:[0-9]{2}:[0-9]{2})");
            if (m.Success)
                thisTitle.Duration = TimeSpan.Parse(m.Groups[1].Value);

            // Get resolution, aspect ratio and FPS for this title
            m = Regex.Match(output.ReadLine(), @"^  \+ size: ([0-9]*)x([0-9]*), pixel aspect: ([0-9]*)/([0-9]*), display aspect: ([0-9]*\.[0-9]*), ([0-9]*\.[0-9]*) fps");
            if (m.Success)
            {
                thisTitle.Resolution = new Size(int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value));
                thisTitle.ParVal = new Size(int.Parse(m.Groups[3].Value), int.Parse(m.Groups[4].Value));
                thisTitle.AspectRatio = Math.Round(float.Parse(m.Groups[5].Value, CultureInfo.InvariantCulture), 2);
                thisTitle.Fps = float.Parse(m.Groups[6].Value, CultureInfo.InvariantCulture);
            }

            // Get autocrop region for this title
            m = Regex.Match(output.ReadLine(), @"^  \+ autocrop: ([0-9]*)/([0-9]*)/([0-9]*)/([0-9]*)");
            if (m.Success)
            {
                thisTitle.AutoCropDimensions = new Cropping
                    {
                        Top = int.Parse(m.Groups[1].Value),
                        Bottom = int.Parse(m.Groups[2].Value),
                        Left = int.Parse(m.Groups[3].Value),
                        Right = int.Parse(m.Groups[4].Value)
                    };
            }

            nextLine = output.ReadLine();
            m = Regex.Match(nextLine, @"^  \+ support opencl:");
            if (m.Success)
            {
                temp = nextLine.Replace("+ support opencl:", string.Empty).Trim();
                if (string.Compare(temp, "yes") == 0)
                    thisTitle.OpenCLSupport = 1;
                else
                    thisTitle.OpenCLSupport = 0;
            }

            nextLine = output.ReadLine();
            m = Regex.Match(nextLine, @"^  \+ support hwd:");
            if (m.Success)
            {
                temp = nextLine.Replace("+ support hwd:", string.Empty).Trim();
                if (string.Compare(temp, "yes") == 0)
                    thisTitle.HWDSupport = 1;
                else
                    thisTitle.HWDSupport = 0;
            }
            thisTitle.Chapters.AddRange(Chapter.ParseList(output));

            thisTitle.AudioTracks.AddRange(AudioHelper.ParseList(output));

            thisTitle.Subtitles.AddRange(Subtitle.ParseList(output));

            return thisTitle;
        }

        /// <summary>
        /// Return a list of parsed titles
        /// </summary>
        /// <param name="output">
        /// The Output
        /// </param>
        /// <param name="isDvdNavDisabled">
        /// The is Dvd Nav Disabled.
        /// </param>
        /// <returns>
        /// A List of titles
        /// </returns>
        public static Title[] ParseList(string output, bool isDvdNavDisabled)
        {
            var titles = new List<Title>();
            var sr = new StringReader(output);

            while (sr.Peek() == '+' || sr.Peek() == ' ')
            {
                // If the the character is a space, then chances are the line
                if (sr.Peek() == ' ') // If the character is a space, then chances are it's the combing detected line.
                    sr.ReadLine(); // Skip over it
                else
                    titles.Add(Parse(sr, isDvdNavDisabled));
            }

            return titles.ToArray();
        }

        /// <summary>
        /// Calcuate the Duration
        /// </summary>
        /// <param name="startPoint">The Start Point (Chapters)</param>
        /// <param name="endPoint">The End Point (Chapters)</param>
        /// <returns>A Timespan</returns>
        public TimeSpan CalculateDuration(int startPoint, int endPoint)
        {
            IEnumerable<Chapter> chapers =
                this.Chapters.Where(c => c.ChapterNumber >= startPoint && c.ChapterNumber <= endPoint);

            TimeSpan duration = TimeSpan.FromSeconds(0.0);
            duration = chapers.Aggregate(duration, (current, chapter) => current + chapter.Duration);

            return duration;
        }

        /// <summary>
        /// Override of the ToString method to provide an easy way to use this object in the UI
        /// </summary>
        /// <returns>A string representing this track in the format: {title #} (00:00:00)</returns>
        public override string ToString()
        {
            if (!string.IsNullOrEmpty(this.Playlist) && !this.Playlist.StartsWith(" "))
            {
                this.Playlist = string.Format(" {0}", this.Playlist);
            }

            return string.Format("{0}{1} ({2:00}:{3:00}:{4:00})", TitleNumber, Playlist, Duration.Hours, Duration.Minutes, Duration.Seconds);
        }
    }
}