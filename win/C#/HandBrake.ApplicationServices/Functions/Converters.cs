namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.Collections.Generic;
    using System.Text.RegularExpressions;

    /// <summary>
    /// A class to convert various things to native C# objects
    /// </summary>
    public class Converters
    {
        /// <summary>
        /// Convert HandBrakes time remaining into a TimeSpan
        /// </summary>
        /// <param name="time">
        /// The time remaining for the encode.
        /// </param>
        /// <returns>
        /// A TimepSpan object
        /// </returns>
        public static TimeSpan EncodeToTimespan(string time)
        {
            TimeSpan converted = new TimeSpan(0, 0, 0, 0);

            Match m = Regex.Match(time.Trim(), @"^([0-9]{2}:[0-9]{2}:[0-9]{2})");
            if (m.Success)
            {
                TimeSpan.TryParse(m.Groups[0].Value, out converted);
            }

            return converted;
        }

        /// <summary>
        /// Video Framerate Converter
        /// </summary>
        private static readonly Dictionary<double, int> vrates = new Dictionary<double, int>
        {
           {5, 5400000},
           {10, 2700000},
           {12, 2250000},
           {15, 1800000},
           {23.976, 1126125},
           {24, 1125000},
           {25, 1080000},
           {29.97, 900900}
        };

        /// <summary>
        /// Convert the desired framerate to the video rate.
        /// </summary>
        /// <param name="framerate">
        /// The framerate.
        /// </param>
        /// <returns>
        /// The Video Rate.
        /// </returns>
        /// <exception cref="ArgumentException">
        /// </exception>
        public static int FramerateToVrate(double framerate)
        {
            if (!vrates.ContainsKey(framerate))
            {
                throw new ArgumentException("Framerate not recognized.", "framerate");
            }

            return vrates[framerate];
        }
    }
}
