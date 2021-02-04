// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakePictureHelpers.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakePictureHelpers type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Interfaces.Model;
    using HandBrake.Interop.Interop.Interfaces.Model.Picture;

    public class HandBrakePictureHelpers
    {
        public enum KeepSetting
        {
            // From common.h
            HB_KEEP_WIDTH = 0x01,
            HB_KEEP_HEIGHT = 0x02,
            HB_KEEP_DISPLAY_ASPECT = 0x04
        }

        public static AnamorphicResult hb_set_anamorphic_size2(PictureSettingsJob job, PictureSettingsTitle title, KeepSetting setting)
        {
            int settingMode = (int)setting + (job.KeepDisplayAspect ? 0x04 : 0);


            hb_rational_t computed_par = new hb_rational_t();
            switch (job.AnamorphicMode)
            {
                case Anamorphic.None:
                    computed_par = new hb_rational_t { den = 1, num = 1 };
                    break;
                case Anamorphic.Custom:
                    computed_par = new hb_rational_t { den = job.ParH, num = job.ParW };
                    break;
                default:
                    computed_par = new hb_rational_t { den = title.ParH, num = title.ParW };
                    break;
            }

            hb_geometry_settings_s uiGeometry = new hb_geometry_settings_s
            {
                crop = new[] { job.Crop.Top, job.Crop.Bottom, job.Crop.Left, job.Crop.Right },
                itu_par = 0,
                keep = settingMode,
                maxWidth = job.MaxWidth,
                maxHeight = job.MaxHeight,
                mode = (int)job.AnamorphicMode,
                modulus = job.Modulus.HasValue ? job.Modulus.Value : 16,
                geometry = new hb_geometry_s { height = job.Height, width = job.Width, par = computed_par }
            };

            hb_geometry_s sourceGeometry = new hb_geometry_s
            {
                width = title.Width,
                height = title.Height,
                par = new hb_rational_t { den = title.ParH, num = title.ParW }
            };

            hb_geometry_s result = HandBrakePictureHelpers.GetAnamorphicSizes(sourceGeometry, uiGeometry);

            int outputWidth = result.width;
            int outputHeight = result.height;
            int outputParWidth = result.par.num;
            int outputParHeight = result.par.den;
            return new AnamorphicResult { OutputWidth = outputWidth, OutputHeight = outputHeight, OutputParWidth = outputParWidth, OutputParHeight = outputParHeight };
        }
        
        internal static hb_geometry_s GetAnamorphicSizes(hb_geometry_s sourceGeometry, hb_geometry_settings_s uiGeometry)
        {
            hb_geometry_s geometry = new hb_geometry_s();

            HBFunctions.hb_set_anamorphic_size2(ref sourceGeometry, ref uiGeometry, ref geometry);

            return geometry;
        }
    }
}
