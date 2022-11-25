// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeHardwareEncoderHelper.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The System Information.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;
    using System.Diagnostics;
    using System.Dynamic;
    using System.Runtime.InteropServices;

    using HandBrake.Interop.Interop.HbLib;

    public class HandBrakeHardwareEncoderHelper
    {
        private static bool? isNvencH264Available; // Local cache to prevent log spam.
        private static bool? isNvencH265Available;

        public static bool IsSafeMode
        {
            get
            {
                try
                {
                    if (RuntimeInformation.ProcessArchitecture == Architecture.Arm64)
                    {
                        return false;
                    }

                    return (HBFunctions.hb_qsv_available() + HBFunctions.hb_vce_h264_available()
                                                           + HBFunctions.hb_nvenc_h264_available()) == -3;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        /* QuickSync Support */

        public static bool IsQsvAvailable
        {
            get
            {
                try
                {
                    // We support Skylake 6th gen and newer. 
                    return HBFunctions.hb_qsv_available() > 0 && QsvHardwareGeneration >= 5; // 5 == Skylake
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static bool IsQsvHyperEncodeAvailable
        {
            get
            {
                try
                {
                    return HBFunctions.hb_qsv_available() > 0 && QsvHyperEncode > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static bool IsQsvAvailableH264
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_H264) > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static bool IsQsvAvailableH265
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_H265) > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static int QsvHardwareGeneration
        {
            get
            {
                try
                {
                    int adapter_index = HBFunctions.hb_qsv_get_adapter_index();
                    int qsv_platform = HBFunctions.hb_qsv_get_platform(adapter_index);
                    int hardware = HBFunctions.hb_qsv_hardware_generation(qsv_platform); 
                    return hardware;
                }
                catch (Exception exc)
                {
                    // Silent failure. -1 means unsupported.
                    Debug.WriteLine(exc);
                    return -1;
                }
            }
        }

        public static int QsvHyperEncode
        {
            get
            {
                try
                {
                    int adapter_index = HBFunctions.hb_qsv_get_adapter_index();
                    return HBFunctions.hb_qsv_hyper_encode_available(adapter_index);
                }
                catch (Exception exc)
                {
                    // Silent failure. -1 means unsupported.
                    Debug.WriteLine(exc);
                    return -1;
                }
            }
        }

        public static bool IsQsvAvailableH26510bit
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_H265_10BIT) > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }
        
        public static bool IsQsvAvailableAV1
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_AV1) > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }
        
        public static bool IsQsvAvailableAV110bit
        {
            get
            {
                try
                {
                    return (HBFunctions.hb_qsv_available() & NativeConstants.HB_VCODEC_QSV_AV1_10BIT) > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }
        
        /* AMD VCE Support */

        public static bool IsVceH264Available
        {
            get
            {
                try
                {
                    return HBFunctions.hb_vce_h264_available() > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static bool IsVceH265Available
        {
            get
            {
                try
                {
                    return HBFunctions.hb_vce_h265_available() > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        /* Nvidia NVEnc Support */

        public static bool IsNVEncH264Available
        {
            get
            {
                try
                {
                    if (isNvencH264Available == null)
                    {
                        isNvencH264Available = HBFunctions.hb_nvenc_h264_available() > 0;
                    }
                    
                    return isNvencH264Available.Value;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static bool IsNVEncH265Available
        {
            get
            {
                try
                {
                    if (!IsNVEncH264Available)
                    {
                        return false;
                    }

                    if (isNvencH265Available == null)
                    {
                        isNvencH265Available = HBFunctions.hb_nvenc_h265_available() > 0;
                    }

                    return isNvencH265Available.Value;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }

        public static bool IsNVDecAvailable
        {
            get
            {
                try
                {
                    if (!IsNVEncH264Available)
                    {
                        return false;
                    }

                    return HBFunctions.hb_check_nvdec_available() > 0;
                }
                catch (Exception)
                {
                    // Silent failure. Typically this means the dll hasn't been built with --enable-qsv
                    return false;
                }
            }
        }
    }
}
