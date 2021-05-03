// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RotateResult.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Picture
{
    public class RotateResult
    {
        public int CropTop { get; set; }

        public int CropBottom { get; set; }

        public int CropLeft { get; set; }

        public int CropRight { get; set; }


        public int PadTop { get; set; }

        public int PadBottom { get; set; }

        public int PadLeft { get; set; }

        public int PadRight { get; set; }


        public int Width { get; set; }

        public int Height { get; set; }

        public int ParNum { get; set; }

        public int ParDen { get; set; }
    }
}
