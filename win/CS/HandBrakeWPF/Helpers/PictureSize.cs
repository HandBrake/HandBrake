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
            public double OutputHeight { get; set; }

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
        /// Calculates job width and height for anamorphic content,
        /// * @param output_width Pointer to returned storage width
        /// * @param output_height Pointer to returned storage height
        /// * @param output_par_width Pointer to returned pixel width
        /// * @param output_par_height Pointer to returned pixel height
        /// </summary>
        /// <param name="job">
        /// The job.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="output_width">
        /// The output_width.
        /// </param>
        /// <param name="output_height">
        /// The output_height.
        /// </param>
        /// <param name="output_par_width">
        /// The output_par_width.
        /// </param>
        /// <param name="output_par_height">
        /// The output_par_height.
        /// </param>
        public static void hb_set_anamorphic_size(PictureSettingsJob job, PictureSettingsTitle title,
                 out int output_width, out int output_height,
                 out int output_par_width, out int output_par_height)
        {
            /* Initialise the Output Width / Height */
            output_width = 0;
            output_height = 0;

            /* Set up some variables to make the math easier to follow. */
            int cropped_width = title.Width - job.Crop.Left - job.Crop.Right;
            int cropped_height = title.Height - job.Crop.Top - job.Crop.Bottom;
            double storage_aspect = cropped_width / (double)cropped_height;
            int mod = job.Modulus.HasValue ? job.Modulus.Value : 16;
            double aspect = title.Aspect;

            long pixel_aspect_width = job.ParW;
            long pixel_aspect_height = job.ParH;

            /* If a source was really NTSC or PAL and the user specified ITU PAR
               values, replace the standard PAR values with the ITU broadcast ones. */
            if (title.Width == 720 && job.ItuPar)
            {
                // convert aspect to a scaled integer so we can test for 16:9 & 4:3
                // aspect ratios ignoring insignificant differences in the LSBs of
                // the floating point representation.
                int iaspect = (int)Math.Round(aspect * 9.0, 0);  // int iaspect = aspect * 9.; TODO CHECK THIS

                /* Handle ITU PARs */
                if (title.Height == 480)
                {
                    /* It's NTSC */
                    if (iaspect == 16)
                    {
                        /* It's widescreen */
                        pixel_aspect_width = 40;
                        pixel_aspect_height = 33;
                    }
                    else if (iaspect == 12)
                    {
                        /* It's 4:3 */
                        pixel_aspect_width = 10;
                        pixel_aspect_height = 11;
                    }
                }
                else if (title.Height == 576)
                {
                    /* It's PAL */
                    if (iaspect == 16)
                    {
                        /* It's widescreen */
                        pixel_aspect_width = 16;
                        pixel_aspect_height = 11;
                    }
                    else if (iaspect == 12)
                    {
                        /* It's 4:3 */
                        pixel_aspect_width = 12;
                        pixel_aspect_height = 11;
                    }
                }
            }

            /* Figure out what width the source would display at. */
            int source_display_width = (int)(cropped_width * (double)pixel_aspect_width / (double)pixel_aspect_height);    // TODO Check casting

            /*
               3 different ways of deciding output dimensions:
                - 1: Strict anamorphic, preserve source dimensions
                - 2: Loose anamorphic, round to mod16 and preserve storage aspect ratio
                - 3: Power user anamorphic, specify everything
            */
            int width, height;
            int maxWidth, maxHeight;

            maxWidth = Macros.MultipleModDown(job.MaxWidth, mod);
            maxHeight = Macros.MultipleModDown(job.MaxHeight, mod);

            switch (job.AnamorphicMode)
            {
                case Anamorphic.Strict:
                    /* Strict anamorphic */
                    output_width = Macros.MultipleMod(cropped_width, 2);
                    output_height = Macros.MultipleMod(cropped_height, 2);
                    // adjust the source PAR for new width/height
                    // new PAR = source PAR * ( old width / new_width ) * ( new_height / old_height )
                    pixel_aspect_width = (long)title.ParW * cropped_width * (output_height);
                    pixel_aspect_height = (long)title.ParH * (output_width) * cropped_height;
                    break;

                case Anamorphic.Loose:
                    /* "Loose" anamorphic.
                        - Uses mod16-compliant dimensions,
                        - Allows users to set the width
                    */
                    width = job.Width;
                    // height: Gets set later, ignore user job->height value

                    /* Gotta handle bounding dimensions.
                       If the width is too big, just reset it with no rescaling.
                       Instead of using the aspect-scaled job height,
                       we need to see if the job width divided by the storage aspect
                       is bigger than the max. If so, set it to the max (this is sloppy).
                       If not, set job height to job width divided by storage aspect.
                    */

                    /* Time to get picture width that divide cleanly.*/
                    width = Macros.MultipleMod(width, mod);

                    if (maxWidth != 0 && (maxWidth < job.Width))
                        width = maxWidth;

                    /* Verify these new dimensions don't violate max height and width settings */
                    height = (int)Math.Round(((double)width / storage_aspect) + 0.5, 0);  // TODO Check Casting and rounding

                    /* Time to get picture height that divide cleanly.*/
                    height = Macros.MultipleMod(height, mod);

                    if (maxHeight != 0 && (maxHeight < height))
                    {
                        height = maxHeight;
                        width = (int)Math.Round(((double)height * storage_aspect) + 0.5, 0); // TODO Check asting and arounding.
                        width = Macros.MultipleMod(width, mod);
                    }

                    /* The film AR is the source's display width / cropped source height.
                       The output display width is the output height * film AR.
                       The output PAR is the output display width / output storage width. */
                    pixel_aspect_width = (long)height * cropped_width * pixel_aspect_width;
                    pixel_aspect_height = (long)width * cropped_height * pixel_aspect_height;

                    /* Pass the results back to the caller */
                    output_width = width;
                    output_height = height;
                    break;

                case Anamorphic.Custom:
                    /* Anamorphic 3: Power User Jamboree
                       - Set everything based on specified values */

                    /* Use specified storage dimensions */
                    storage_aspect = job.Width / (double)job.Height;
                    width = job.Width;
                    height = job.Height;

                    /* Time to get picture dimensions that divide cleanly.*/
                    width = Macros.MultipleMod(width, mod);
                    height = Macros.MultipleMod(height, mod);

                    /* Bind to max dimensions */
                    if (maxWidth != 0 && width > maxWidth)
                    {
                        width = maxWidth;
                        // If we are keeping the display aspect, then we are going
                        // to be modifying the PAR anyway.  So it's preferred
                        // to let the width/height stray some from the original
                        // requested storage aspect.
                        //
                        // But otherwise, PAR and DAR will change the least
                        // if we stay as close as possible to the requested
                        // storage aspect.
                        if (!job.KeepDisplayAspect)
                        {
                            height = (int)Math.Round(((double)width / storage_aspect) + 0.5, 0); // TODO Check rounding and casting
                            height = Macros.MultipleMod(height, mod);
                        }
                    }
                    if (maxHeight != 0 && height > maxHeight)
                    {
                        height = maxHeight;
                        // Ditto, see comment above
                        if (!job.KeepDisplayAspect)
                        {
                            width = (int)Math.Round(((double)height * storage_aspect) + 0.5, 0); // TODO check Rounding and casting
                            width = Macros.MultipleMod(width, mod);
                        }
                    }

                    /* That finishes the storage dimensions. On to display. */
                    if (job.DarWidth != 0 && job.DarHeight != 0)
                    {
                        /* We need to adjust the PAR to produce this aspect. */
                        pixel_aspect_width = (long)height * job.DarWidth / job.DarHeight;
                        pixel_aspect_height = width;
                    }
                    else
                    {
                        /* If we're doing ana 3 and not specifying a DAR, care needs to be taken.
                           This indicates a PAR is potentially being set by the interface. But
                           this is an output PAR, to correct a source, and it should not be assumed
                           that it properly creates a display aspect ratio when applied to the source,
                           which could easily be stored in a different resolution. */
                        if (job.KeepDisplayAspect)
                        {
                            /* We can ignore the possibility of a PAR change */
                            pixel_aspect_width = (int)Math.Round((long)height * ((double)source_display_width / (double)cropped_height), 0); // TODO Check rounding and casting
                            pixel_aspect_height = width;
                        }
                        else
                        {
                            int output_display_width = (int)Math.Round(width * (double)pixel_aspect_width / (double)pixel_aspect_height, 0); // TODO Check rounding and casting.
                            pixel_aspect_width = output_display_width;
                            pixel_aspect_height = width;
                        }
                    }

                    /* Back to caller */
                    output_width = width;
                    output_height = height;
                    break;
            }

            /* While x264 is smart enough to reduce fractions on its own, libavcodec
             * needs some help with the math, so lose superfluous factors. */
            hb_limit_rational64(out pixel_aspect_width, out pixel_aspect_height,
                                pixel_aspect_width, pixel_aspect_height, 65535);
            hb_reduce(out output_par_width, out output_par_height,
                       (int)pixel_aspect_width, (int)pixel_aspect_height);
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
        public static AnamorphicResult hb_set_anamorphic_size_native(PictureSettingsJob job, PictureSettingsTitle title)
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
                                                     mode = (int)job.AnamorphicMode,
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

        /// <summary>
        /// The hb_limit_rational 64.
        /// </summary>
        /// <param name="x">
        /// The x.
        /// </param>
        /// <param name="y">
        /// The y.
        /// </param>
        /// <param name="num">
        /// The num.
        /// </param>
        /// <param name="den">
        /// The den.
        /// </param>
        /// <param name="limit">
        /// The limit.
        /// </param>
        private static void hb_limit_rational64(out long x, out long y, long num, long den, long limit)
        {
            hb_reduce64(out num, out den, num, den);
            if (num < limit && den < limit)
            {
                x = num;
                y = den;
                return;
            }

            if (num > den)
            {
                double div = (double)limit / num;
                num = limit;
                den *= (int)Math.Round(div, 0); // TODO Check cast and rounding.
            }
            else
            {
                double div = (double)limit / den;
                den = limit;
                num *= (int)Math.Round(div, 0); // TODO Check cast and rounding.
            }

            x = num;
            y = den;
        }

        /// <summary>
        /// Given a numerator (num) and a denominator (den), reduce them to an
        /// equivalent fraction and store the result in x and y.
        /// </summary>
        /// <param name="x">
        /// The x.
        /// </param>
        /// <param name="y">
        /// The y.
        /// </param>
        /// <param name="num">
        /// The num.
        /// </param>
        /// <param name="den">
        /// The den.
        /// </param>
        private static void hb_reduce64(out long x, out long y, long num, long den)
        {
            // find the greatest common divisor of num & den by Euclid's algorithm
            long n = num, d = den;
            while (d != 0)
            {
                long t = d;
                d = n % d;
                n = t;
            }

            // at this point n is the gcd. if it's non-zero remove it from num
            // and den. Otherwise just return the original values.
            if (n != 0)
            {
                num /= n;
                den /= n;
            }

            x = num;
            y = den;
        }

        /// <summary>
        /// Given a numerator (num) and a denominator (den), reduce them to an
        /// equivalent fraction and store the result in x and y.
        /// </summary>
        /// <param name="x">
        /// The x.
        /// </param>
        /// <param name="y">
        /// The y.
        /// </param>
        /// <param name="num">
        /// The num.
        /// </param>
        /// <param name="den">
        /// The den.
        /// </param>
        private static void hb_reduce(out int x, out int y, int num, int den)
        {
            // find the greatest common divisor of num & den by Euclid's algorithm
            int n = num, d = den;
            while (d != 0)
            {
                int t = d;
                d = n % d;
                n = t;
            }

            // at this point n is the gcd. if it's non-zero remove it from num
            // and den. Otherwise just return the original values.
            if (n != 0)
            {
                x = num / n;
                y = den / n;
            }
            else
            {
                x = num;
                y = den;
            }
        }
    }
}
