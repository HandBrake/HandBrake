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
        /// The hb_set_anamorphic_size_native.
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <returns>
        /// The <see cref="AnamorphicResult"/> object.
        /// </returns>
        public static AnamorphicResult hb_set_anamorphic_size(PictureSettingsJob job, PictureSettingsTitle title)
        {
            int outputHeight = 0;
            int outputParHeight = 0;
            int outputParWidth = 0;
            int outputWidth = 0;

            hb_job_s nativeJob = new hb_job_s
                                     {
                                         modulus = job.Modulus.HasValue ? job.Modulus.Value : 16,
                                         anamorphic =
                                             new hb_anamorphic_substruct
                                                 {
                                                     par_width = job.ParW,
                                                     par_height = job.ParH,
                                                     itu_par = 0,
                                                     mode = (hb_anamorphic_mode_t)job.AnamorphicMode,
                                                     dar_width = 0,
                                                     dar_height = 0,
                                                     keep_display_aspect = job.KeepDisplayAspect ? 1 : 0
                                                 },
                                         maxWidth = title.Width,
                                         maxHeight = title.Height,
                                         keep_ratio = job.KeepDisplayAspect ? 1 : 0,
                                         width = job.Width,
                                         height = job.Height,
                                         crop = new[] { job.Crop.Top, job.Crop.Bottom, job.Crop.Left, job.Crop.Right }
                                     };

            hb_title_s title_s = new hb_title_s
                                     {
                                         crop = new[] { job.Crop.Top, job.Crop.Bottom, job.Crop.Left, job.Crop.Right },
                                         width = title.Width,
                                         height = title.Height,
                                         pixel_aspect_width = title.ParW,
                                         pixel_aspect_height = title.ParH,
                                         aspect = 0
                                     };

            IntPtr pointer = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(hb_title_s)));
            Marshal.StructureToPtr(title_s, pointer, false);
            nativeJob.title = pointer;

            HBFunctions.hb_set_anamorphic_size(
                ref nativeJob, ref outputWidth, ref outputHeight, ref outputParWidth, ref outputParHeight);

            return new AnamorphicResult { OutputWidth = outputWidth, OutputHeight = outputHeight, OutputParWidth = outputParWidth, OutputParHeight = outputParHeight };
        }
    }
}
