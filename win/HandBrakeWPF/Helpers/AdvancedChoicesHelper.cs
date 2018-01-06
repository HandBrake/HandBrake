// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AdvancedChoicesHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The advanced choices.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System.Collections.Generic;
    using System.Globalization;

    using HandBrakeWPF.Model;

    /// <summary>
    /// The advanced choices.
    /// </summary>
    public static class AdvancedChoicesHelper
    {
        #region Constants and Fields

        /// <summary>
        /// The adaptive b frames.
        /// </summary>
        private static List<AdvancedChoice> adaptiveBFrames;

        /// <summary>
        /// The analysis.
        /// </summary>
        private static List<AdvancedChoice> analysis;

        /// <summary>
        /// The b frames.
        /// </summary>
        private static List<AdvancedChoice> bFrames;

        /// <summary>
        /// The deblocking strength.
        /// </summary>
        private static List<AdvancedChoice> deblockingStrength;

        /// <summary>
        /// The deblocking threshold.
        /// </summary>
        private static List<AdvancedChoice> deblockingThreshold;

        /// <summary>
        /// The direct prediction.
        /// </summary>
        private static List<AdvancedChoice> directPrediction;

        /// <summary>
        /// The motion estimation method.
        /// </summary>
        private static List<AdvancedChoice> motionEstimationMethod;

        /// <summary>
        /// The motion estimation range.
        /// </summary>
        private static List<AdvancedChoice> motionEstimationRange;

        /// <summary>
        /// The pyramidal b frames.
        /// </summary>
        private static List<AdvancedChoice> pyramidalBFrames;

        /// <summary>
        /// The reference frames.
        /// </summary>
        private static List<AdvancedChoice> referenceFrames;

        /// <summary>
        /// The subpixel motion estimation.
        /// </summary>
        private static List<AdvancedChoice> subpixelMotionEstimation;

        /// <summary>
        /// The trellis.
        /// </summary>
        private static List<AdvancedChoice> trellis;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        /// Initializes static members of the <see cref="AdvancedChoicesHelper"/> class.
        /// </summary>
        static AdvancedChoicesHelper()
        {
            referenceFrames = CreateNumberList(1, 16, defaultNumber: 3);
            bFrames = CreateNumberList(0, 16, defaultNumber: 3);

            pyramidalBFrames = new List<AdvancedChoice>
                {
                    new AdvancedChoice { Label = "Off", Value = "none" },
                    new AdvancedChoice { Label = "Normal (Default)", IsDefault = true },
                    new AdvancedChoice { Label = "Strict", Value = "strict" }
                };

            adaptiveBFrames = new List<AdvancedChoice>
                {
                    new AdvancedChoice { Label = "Off", Value = "0" },
                    new AdvancedChoice { Label = "Fast (Default)", Value = "1", IsDefault = true },
                    new AdvancedChoice { Label = "Optimal", Value = "2" }
                };

            directPrediction = new List<AdvancedChoice>
                {
                    new AdvancedChoice { Label = "None", Value = "none" },
                    new AdvancedChoice { Label = "Spatial (Default)", Value = "spatial", IsDefault = true },
                    new AdvancedChoice { Label = "Temporal", Value = "temporal" },
                    new AdvancedChoice { Label = "Automatic", Value = "auto" }
                };

            motionEstimationMethod = new List<AdvancedChoice>
                {
                    new AdvancedChoice { Label = "Diamond", Value = "dia" },
                    new AdvancedChoice { Label = "Hexagon (Default)", Value = "hex", IsDefault = true },
                    new AdvancedChoice { Label = "Uneven Multi-Hexagon", Value = "umh" },
                    new AdvancedChoice { Label = "Exhaustive", Value = "esa" },
                    new AdvancedChoice { Label = "Transformed Exhaustive", Value = "tesa" },
                };

            subpixelMotionEstimation = new List<AdvancedChoice>
                {
                    new AdvancedChoice { Label = "0: SAD, no subpel (super fast!)", Value = "0" },
                    new AdvancedChoice { Label = "1: SAD, qpel", Value = "1" },
                    new AdvancedChoice { Label = "2: SATD, qpel", Value = "2" },
                    new AdvancedChoice { Label = "3: SATD, multi-qpel", Value = "3" },
                    new AdvancedChoice { Label = "4: SATD, qpel on all", Value = "4" },
                    new AdvancedChoice { Label = "5: SATD, multi-qpel on all", Value = "5" },
                    new AdvancedChoice { Label = "6: RD in I/P-frames", Value = "6" },
                    new AdvancedChoice { Label = "7: RD in all frames (Default)", Value = "7", IsDefault = true },
                    new AdvancedChoice { Label = "8: RD refine in I/P-frames", Value = "8" },
                    new AdvancedChoice { Label = "9: RD refine in all frames", Value = "9" },
                    new AdvancedChoice { Label = "10: QPRD in all frames", Value = "10" },
                    new AdvancedChoice { Label = "11: No early terminations in analysis", Value = "11" },
                };

            // subpixelMotionEstimation = CreateNumberList(0, 9, defaultNumber: 7);
            motionEstimationRange = CreateNumberList(4, 64, defaultNumber: 16);

            analysis = new List<AdvancedChoice>
                {
                    new AdvancedChoice { Label = "Most (Default)", IsDefault = true },
                    new AdvancedChoice { Label = "None", Value = "none" },
                    new AdvancedChoice { Label = "Some", Value = "i4x4,i8x8", },
                    new AdvancedChoice { Label = "All", Value = "all" }
                };

            trellis = new List<AdvancedChoice>
                {
                    new AdvancedChoice { Label = "Off", Value = "0" },
                    new AdvancedChoice { Label = "Encode Only (Default)", Value = "1", IsDefault = true },
                    new AdvancedChoice { Label = "Always", Value = "2" }
                };

            deblockingStrength = CreateNumberList(-6, 6, defaultNumber: 0);
            deblockingThreshold = CreateNumberList(-6, 6, defaultNumber: 0);
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets AdaptiveBFrames.
        /// </summary>
        public static List<AdvancedChoice> AdaptiveBFrames
        {
            get
            {
                return adaptiveBFrames;
            }
        }

        /// <summary>
        /// Gets Analysis.
        /// </summary>
        public static List<AdvancedChoice> Analysis
        {
            get
            {
                return analysis;
            }
        }

        /// <summary>
        /// Gets BFrames.
        /// </summary>
        public static List<AdvancedChoice> BFrames
        {
            get
            {
                return bFrames;
            }
        }

        /// <summary>
        /// Gets DeblockingStrength.
        /// </summary>
        public static List<AdvancedChoice> DeblockingStrength
        {
            get
            {
                return deblockingStrength;
            }
        }

        /// <summary>
        /// Gets DeblockingThreshold.
        /// </summary>
        public static List<AdvancedChoice> DeblockingThreshold
        {
            get
            {
                return deblockingThreshold;
            }
        }

        /// <summary>
        /// Gets DirectPrediction.
        /// </summary>
        public static List<AdvancedChoice> DirectPrediction
        {
            get
            {
                return directPrediction;
            }
        }

        /// <summary>
        /// Gets MotionEstimationMethod.
        /// </summary>
        public static List<AdvancedChoice> MotionEstimationMethod
        {
            get
            {
                return motionEstimationMethod;
            }
        }

        /// <summary>
        /// Gets MotionEstimationRange.
        /// </summary>
        public static List<AdvancedChoice> MotionEstimationRange
        {
            get
            {
                return motionEstimationRange;
            }
        }

        /// <summary>
        /// Gets PyramidalBFrames.
        /// </summary>
        public static List<AdvancedChoice> PyramidalBFrames
        {
            get
            {
                return pyramidalBFrames;
            }
        }

        /// <summary>
        /// Gets ReferenceFrames.
        /// </summary>
        public static List<AdvancedChoice> ReferenceFrames
        {
            get
            {
                return referenceFrames;
            }
        }

        /// <summary>
        /// Gets SubpixelMotionEstimation.
        /// </summary>
        public static List<AdvancedChoice> SubpixelMotionEstimation
        {
            get
            {
                return subpixelMotionEstimation;
            }
        }

        /// <summary>
        /// Gets Trellis.
        /// </summary>
        public static List<AdvancedChoice> Trellis
        {
            get
            {
                return trellis;
            }
        }

        #endregion

        #region Methods

        /// <summary>
        /// The add range.
        /// </summary>
        /// <param name="list">
        /// The list.
        /// </param>
        /// <param name="lower">
        /// The lower.
        /// </param>
        /// <param name="upper">
        /// The upper.
        /// </param>
        /// <param name="defaultNumber">
        /// The default number.
        /// </param>
        private static void AddRange(List<AdvancedChoice> list, int lower, int upper, int defaultNumber)
        {
            for (int i = lower; i <= upper; i++)
            {
                if (i == defaultNumber)
                {
                    list.Add(
                        new AdvancedChoice
                            {
                                IsDefault = true, 
                                Label = i.ToString() + " (Default)", 
                                Value = i.ToString(CultureInfo.InvariantCulture)
                            });
                }
                else
                {
                    list.Add(
                        new AdvancedChoice { Label = i.ToString(), Value = i.ToString(CultureInfo.InvariantCulture) });
                }
            }
        }

        /// <summary>
        /// The create number list.
        /// </summary>
        /// <param name="lower">
        /// The lower.
        /// </param>
        /// <param name="upper">
        /// The upper.
        /// </param>
        /// <param name="defaultNumber">
        /// The default number.
        /// </param>
        /// <returns>
        /// List of Advanced Choices Options.
        /// </returns>
        private static List<AdvancedChoice> CreateNumberList(int lower, int upper, int defaultNumber)
        {
            var list = new List<AdvancedChoice>();
            AddRange(list, lower, upper, defaultNumber);

            return list;
        }

        #endregion
    }
}