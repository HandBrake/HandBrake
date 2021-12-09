// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakePictureHelpers.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Wrapper around functions in libhb.
//   See common.h for struct's / API
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    public class HandBrakePictureHelpers
    {
        [Flags]
        public enum KeepSetting
        {
            // From common.h
            HB_KEEP_WIDTH = 0x01,
            HB_KEEP_HEIGHT = 0x02,
            HB_KEEP_DISPLAY_ASPECT = 0x04,
            HB_KEEP_DISPLAY_WIDTH = 0x08,
            HB_KEEP_PAD = 0x10,
        }
        
        [Flags]
        public enum FlagsSetting
        {
            HB_GEO_SCALE_UP = 0x01,
            HB_GEO_SCALE_BEST = 0x02,
        }

        public static AnamorphicResult GetAnamorphicSize(PictureSettingsJob job, PictureSettingsTitle sourceTitle, KeepSetting keepSetting, FlagsSetting flagSetting)
        {
            hb_rational_t computedPar;
            switch (job.AnamorphicMode)
            {
                case Anamorphic.None:
                    computedPar = new hb_rational_t { den = 1, num = 1 };
                    break;
                case Anamorphic.Custom:
                    computedPar = new hb_rational_t { den = job.ParH, num = job.ParW };
                    break;
                default:
                    computedPar = new hb_rational_t { den = sourceTitle.ParH, num = sourceTitle.ParW };
                    break;
            }

            hb_geometry_settings_s uiGeometry = new hb_geometry_settings_s
            {
                crop = new[] { job.Crop.Top, job.Crop.Bottom, job.Crop.Left, job.Crop.Right },
                pad = new[] { job.Pad.Top, job.Pad.Bottom, job.Pad.Left, job.Pad.Right },
                flags = (int)flagSetting,
                itu_par = 0,
                keep = (int)keepSetting,
                maxWidth = job.MaxWidth,
                maxHeight = job.MaxHeight,
                mode = (int)job.AnamorphicMode,
                modulus = 2,
                geometry = new hb_geometry_s { height = job.Height, width = job.Width, par = computedPar },
                displayWidth = job.DarWidth,
                displayHeight = job.DarHeight
            };

            // If we are rotated, the source title geometry must also be rotated. 
            int titleWidth, titleHeight, titleParW, titleParH;
            if (job.RotateAngle != 0)
            {
                PictureSettingsJob titleRepresentation = new PictureSettingsJob
                                                         {
                                                             Width = sourceTitle.Width,
                                                             Height = sourceTitle.Height,
                                                             RotateAngle = job.RotateAngle,
                                                             Hflip = job.Hflip,
                                                             ParW = sourceTitle.ParW,
                                                             ParH = sourceTitle.ParH,
                                                             Crop = new Cropping(),
                                                             Pad = new Padding()
                                                         };
                RotateResult titleRotation = HandBrakePictureHelpers.RotateGeometry(titleRepresentation);
                titleWidth = titleRotation.Width;
                titleHeight = titleRotation.Height;
                titleParW = titleRotation.ParNum;
                titleParH = titleRotation.ParDen;
            }
            else
            {
                titleWidth = sourceTitle.Width;
                titleHeight = sourceTitle.Height;
                titleParW = sourceTitle.ParW;
                titleParH = sourceTitle.ParH;
            }

            hb_geometry_s sourceGeometry = new hb_geometry_s
            {
                width = titleWidth,
                height = titleHeight,
                par = new hb_rational_t { den = titleParH, num = titleParW }
            };

            hb_geometry_s result = new hb_geometry_s();

            HBFunctions.hb_set_anamorphic_size2(ref sourceGeometry, ref uiGeometry, ref result);

            int outputWidth = result.width;
            int outputHeight = result.height;
            int outputParWidth = result.par.num;
            int outputParHeight = result.par.den;
            return new AnamorphicResult { OutputWidth = outputWidth, OutputHeight = outputHeight, OutputParWidth = outputParWidth, OutputParHeight = outputParHeight };
        }

        public static RotateResult RotateGeometry(PictureSettingsJob job)
        {
            hb_geometry_crop_s geometry = new hb_geometry_crop_s();
            geometry.crop = new[] { job.Crop.Top, job.Crop.Bottom, job.Crop.Left, job.Crop.Right };
            geometry.pad = new[] { job.Pad.Top, job.Pad.Bottom, job.Pad.Left, job.Pad.Right };
            geometry.geometry = new hb_geometry_s
                                {
                                    width = job.Width,
                                    height = job.Height,
                                    par = new hb_rational_t { num = job.ParW, den = job.ParH }
                                };

            // Undo any previous rotation so that we are back at 0.
            // Normally hflip is applied, then rotation.
            // To revert, must apply rotation then hflip.
            if (job.PreviousRotation.HasValue && job.PreviousRotation != 0)
            {
                HBFunctions.hb_rotate_geometry(ref geometry, ref geometry, 360 - job.PreviousRotation.Value, 0);
            }

            if (job.PreviousHflip.HasValue && job.PreviousHflip == 1)
            {
                HBFunctions.hb_rotate_geometry(ref geometry, ref geometry, 0, 1);
            }
            
            // Apply the new rotation and Horizontal flip value.
            HBFunctions.hb_rotate_geometry(ref geometry, ref geometry, job.RotateAngle, job.Hflip);

            RotateResult processedResult = new RotateResult
                                           {
                                               CropTop = geometry.crop[0],
                                               CropBottom = geometry.crop[1],
                                               CropLeft = geometry.crop[2],
                                               CropRight = geometry.crop[3],
                                               PadTop = geometry.pad[0],
                                               PadBottom = geometry.pad[1],
                                               PadLeft = geometry.pad[2],
                                               PadRight = geometry.pad[3],
                                               Width = geometry.geometry.width,
                                               Height = geometry.geometry.height,
                                               ParNum = geometry.geometry.par.num,
                                               ParDen = geometry.geometry.par.den
                                           };

            return processedResult;
        }

        public static string GetNiceDisplayAspect(double displayWidth, double displayHeight)
        {
            string result;

            double iaspect = displayWidth * 9 / displayHeight;
            if (displayWidth / displayHeight > 1.9)
            {
                // x.x:1
                result = string.Format("{0}:1", Math.Round(displayWidth / displayHeight, 2));
            }
            else if (iaspect >= 15)
            {
                // x.x:9
                result = string.Format("{0}:9", Math.Round(displayWidth * 9 / displayHeight, 2));
            }
            else if (iaspect >= 9)
            {
                // x.x:3
                result = string.Format("{0}:3", Math.Round(displayWidth * 3 / displayHeight, 2));
            }
            else
            {
                // 1:x.x
                result = string.Format("1:{0}", Math.Round(displayHeight / displayWidth, 2));
            }

            return result;
        }
    }
}
