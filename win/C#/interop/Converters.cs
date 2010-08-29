using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace HandBrake.Interop
{
    public static class Converters
    {
        private static Dictionary<double, int> vrates = new Dictionary<double, int>
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
