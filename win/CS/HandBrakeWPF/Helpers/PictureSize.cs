// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PictureSize.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the PictureSize type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    using HandBrake.Interop.HbLib;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.Model.Encoding;

    /// <summary>
    /// The picture size Helpers
    /// </summary>
    public class PictureSize
    {
        /// <summary>
        /// The picture settings job.
        /// </summary>
        public class PictureSettingsJob
        {
            /// <summary>
            /// Gets or sets the crop.
            /// </summary>
            public Cropping Crop { get; set; }

            /// <summary>
            /// Gets or sets the modulus.
            /// </summary>
            public int? Modulus { get; set; }

            /// <summary>
            /// Gets or sets the par w.
            /// </summary>
            public int ParW { get; set; }

            /// <summary>
            /// Gets or sets the par h.
            /// </summary>
            public int ParH { get; set; }

            /// <summary>
            /// Gets or sets a value indicating whether itu par.
            /// </summary>
            public bool ItuPar { get; set; }

            /// <summary>
            /// Gets or sets the width.
            /// </summary>
            public int Width { get; set; }

            /// <summary>
            /// Gets or sets the height.
            /// </summary>
            public int Height { get; set; }

            /// <summary>
            /// Gets or sets the anamorphic mode.
            /// </summary>
            public Anamorphic AnamorphicMode { get; set; }

            /// <summary>
            /// Gets or sets the max width.
            /// </summary>
            public int MaxWidth { get; set; }

            /// <summary>
            /// Gets or sets the max height.
            /// </summary>
            public int MaxHeight { get; set; }

            /// <summary>
            /// Gets or sets a value indicating whether keep display aspect.
            /// </summary>
            public bool KeepDisplayAspect { get; set; }

            /// <summary>
            /// Gets or sets the dar width.
            /// </summary>
            public int DarWidth { get; set; }

            /// <summary>
            /// Gets or sets the dar height.
            /// </summary>
            public int DarHeight { get; set; }
        }

        /// <summary>
        /// The picture settings title.
        /// </summary>
        public class PictureSettingsTitle
        {
            /// <summary>
            /// Gets or sets the width.
            /// </summary>
            public int Width { get; set; }

            /// <summary>
            /// Gets or sets the height.
            /// </summary>
            public int Height { get; set; }

            /// <summary>
            /// Gets or sets the par w.
            /// </summary>
            public int ParW { get; set; }

            /// <summary>
            /// Gets or sets the par h.
            /// </summary>
            public int ParH { get; set; }

            /// <summary>
            /// Gets or sets the aspect.
            /// </summary>
            public double Aspect { get; set; }
        }

        /// <summary>
        /// The anamorphic result.
        /// </summary>
        public class AnamorphicResult
        {
            /// <summary>
            /// Gets or sets the output width.
            /// </summary>
            public int OutputWidth { get; set; }

            /// <summary>
            /// Gets or sets the output height.
            /// </summary>
            public int OutputHeight { get; set; }

            /// <summary>
            /// Gets or sets the output par width.
            /// </summary>
            public double OutputParWidth { get; set; }

            /// <summary>
            /// Gets or sets the output par height.
            /// </summary>
            public double OutputParHeight { get; set; }
        }

        /// <summary>
        /// The keep setting.
        /// </summary>
        public enum KeepSetting
        {
            HB_KEEP_WIDTH = 0x01,
            HB_KEEP_HEIGHT = 0x02,
            HB_KEEP_DISPLAY_ASPECT = 0x04
        }

        /// <summary>
        /// The hb_set_anamorphic_size 2.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="setting">
        /// The setting.
        /// </param>
        /// <returns>
        /// The <see cref="AnamorphicResult"/>.
        /// </returns>
        public static AnamorphicResult hb_set_anamorphic_size2(PictureSettingsJob job, PictureSettingsTitle title, KeepSetting setting)
        {
            int outputHeight = 0;
            int outputParHeight = 0;
            int outputParWidth = 0;
            int outputWidth = 0;

            int settingMode = (int)setting + (job.KeepDisplayAspect ? 0x04 : 0);

            hb_ui_geometry_s uiGeometry = new hb_ui_geometry_s
            {
                crop = new[] { job.Crop.Top, job.Crop.Bottom, job.Crop.Left, job.Crop.Right },
                dar = new hb_rational_t { den = 0, num = 0 },
                height = job.Height,
                itu_par = 0,
                keep = settingMode,
                maxWidth = job.MaxWidth,
                maxHeight = job.MaxHeight,
                mode = (int)(hb_anamorphic_mode_t)job.AnamorphicMode,
                modulus = job.Modulus.HasValue ? job.Modulus.Value : 16,
                par = job.AnamorphicMode != Anamorphic.Custom ? new hb_rational_t { den = title.ParH, num = title.ParW } : new hb_rational_t { den = job.ParH, num = job.ParW },
                width = job.Width,
            };

            hb_geometry_s sourceGeometry = new hb_geometry_s
            {
                width = title.Width,
                height = title.Height,
                par = new hb_rational_t { den = title.ParH, num = title.ParW }
            };

            hb_geometry_s result = new hb_geometry_s();

            HBFunctions.hb_set_anamorphic_size2(ref sourceGeometry, ref uiGeometry, ref result);

            outputWidth = result.width;
            outputHeight = result.height;
            outputParWidth = result.par.den;
            outputParHeight = result.par.num;
            Debug.WriteLine("hb_set_anamorphic_size2: {0}x{1}", outputWidth, outputHeight);
            return new AnamorphicResult { OutputWidth = outputWidth, OutputHeight = outputHeight, OutputParWidth = outputParWidth, OutputParHeight = outputParHeight };
        }
    }
}
