// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PictureSettingsJob.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model.Picture
{
    public class PictureSettingsJob
    {
        public Cropping Crop { get; set; }

        public Padding Pad { get; set; }

        public int ParW { get; set; }

        public int ParH { get; set; }

        public bool ItuPar { get; set; }

        public int Width { get; set; }

        public int Height { get; set; }

        public Anamorphic AnamorphicMode { get; set; }

        public int MaxWidth { get; set; }

        public int MaxHeight { get; set; }

        public bool KeepDisplayAspect { get; set; }

        public int DarWidth { get; set; }

        public int DarHeight { get; set; }
        

        public int? PreviousRotation { get; set;  }

        public int RotateAngle { get; set; }

        public int? PreviousHflip { get; set; }

        public int Hflip { get; set; }
    }
}
