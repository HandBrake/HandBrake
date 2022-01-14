﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PaddingFilter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PaddingFilter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Model.Filters
{
    public class PaddingFilter
    {
        public PaddingFilter()
        {
        }

        public PaddingFilter(PaddingFilter filter)
        {
            this.Enabled = filter.Enabled;
            this.Color = filter.Color;
            this.X = filter.X;
            this.Y = filter.Y;
            this.W = filter.W;
            this.H = filter.H;
        }

        public bool Enabled { get; set; }

        public string Color { get; set; }

        public string Mode 
        {
            get
            {
                // TODO support "fillWidth" and "fillHeight"
                return this.Enabled ? "custom" : "none";
            }
        }

        /* (X,Y) position of the picture on the frame */

        public int X { get; set; } // Left

        public int Y { get; set; } // Top

        /* The Width and Height of the padding */

        public int W { get; set; } // L +R

        public int H { get; set; } // T + B
        
        /* Helper methods to work out  the Bottom and Right real padding values.*/ 

        public int Bottom
        {
            get
            {
                return this.H - this.Y;
            }
        }

        public int Right
        {
            get
            {
                return this.W - this.X;
            }
        }

        /* Set from imported preset*/

        public void Set(int top, int bottom, int left, int right, string colour, string mode)
        {
            // Figure the X,Y coordinate 
            this.X = left;
            this.Y = top;

            // Calculate the total padding
            this.W = left + right;
            this.H = top + bottom;

            this.Color = colour;

            switch (mode)
            {
                case "none":
                    this.Enabled = false;
                    break;
                case "custom":
                    this.Enabled = true;
                    break;
                case "fillWidth": // TODO, not supported yet
                    this.Enabled = true;
                    break;
                case "fillHeight": // TODO, not supported yet
                    this.Enabled = true;
                    break;
            }
        }
    }
}
