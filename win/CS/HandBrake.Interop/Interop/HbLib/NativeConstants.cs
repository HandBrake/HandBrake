// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NativeConstants.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the NativeConstants type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

// Constants values must match the libhb/handbrake/common.h otherwise Windows GUI issues

namespace HandBrake.Interop.Interop.HbLib
{
    public class NativeConstants
    {
        // Audio encoders
        public const uint HB_ACODEC_FFALAC = 0x00000080;
        public const uint HB_ACODEC_FFALAC24 = 0x00000100;
        public const uint HB_ACODEC_AC3 = 0x00000800;
        public const uint HB_ACODEC_DCA = 0x00002000;
        public const uint HB_ACODEC_FFAAC = 0x00010000;
        public const uint HB_ACODEC_DCA_HD = 0x00040000;
        public const uint HB_ACODEC_MP3 = 0x00080000;
        public const uint HB_ACODEC_FFFLAC = 0x00100000;
        public const uint HB_ACODEC_FFFLAC_24 = 0x00200000;
        public const uint HB_ACODEC_FFEAC3 = 0x01000000;
        public const uint HB_ACODEC_FFTRUEHD = 0x02000000;
        public const uint HB_ACODEC_OPUS = 0x04000000;
        public const uint HB_ACODEC_MP2 = 0x08000000;
        public const uint HB_ACODEC_PASS_FLAG = 0x40000000;
        public const uint HB_ACODEC_PASS_MASK = (HB_ACODEC_AC3 | HB_ACODEC_DCA | HB_ACODEC_DCA_HD | HB_ACODEC_FFAAC | HB_ACODEC_FFEAC3 | HB_ACODEC_FFALAC | HB_ACODEC_FFFLAC | HB_ACODEC_MP2 | HB_ACODEC_MP3 | HB_ACODEC_FFTRUEHD | HB_ACODEC_OPUS);

        public const uint HB_ACODEC_MASK = 0x0FFFFF81;
        public const uint HB_ACODEC_AUTO_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_PASS_MASK);
        public const uint HB_ACODEC_ANY = (HB_ACODEC_PASS_FLAG | HB_ACODEC_MASK);
        public const uint HB_ACODEC_AAC_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFAAC);
        public const uint HB_ACODEC_AC3_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_AC3);
        public const uint HB_ACODEC_ALAC_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFALAC);
        public const uint HB_ACODEC_DCA_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_DCA);
        public const uint HB_ACODEC_DCA_HD_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_DCA_HD);
        public const uint HB_ACODEC_EAC3_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFEAC3);
        public const uint HB_ACODEC_FLAC_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFFLAC);
        public const uint HB_ACODEC_MP3_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_MP3);
        public const uint HB_ACODEC_TRUEHD_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_FFTRUEHD);
        public const uint HB_ACODEC_MP2_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_MP2);
        public const uint HB_ACODEC_OPUS_PASS = (HB_ACODEC_PASS_FLAG | HB_ACODEC_OPUS);

        // Video Encoders
        public const uint HB_VCODEC_AV1_MASK  = 0x40000000;
        public const uint HB_VCODEC_H264_MASK = 0x20000000;
        public const uint HB_VCODEC_H265_MASK = 0x10000000;
        public const uint HB_VCODEC_QSV_MASK  = 0x00040000;

        public const uint HB_VCODEC_QSV_H264 = (0x00000060 | HB_VCODEC_QSV_MASK | HB_VCODEC_H264_MASK);
        public const uint HB_VCODEC_QSV_H265 = (0x00000061 | HB_VCODEC_QSV_MASK | HB_VCODEC_H265_MASK);
        public const uint HB_VCODEC_QSV_H265_10BIT = (0x00000062 | HB_VCODEC_QSV_MASK | HB_VCODEC_H265_MASK);
        public const uint HB_VCODEC_QSV_AV1 = (0x00000070 | HB_VCODEC_QSV_MASK | HB_VCODEC_AV1_MASK);
        public const uint HB_VCODEC_QSV_AV1_10BIT = (0x08000071 | HB_VCODEC_QSV_MASK | HB_VCODEC_AV1_MASK);

        // Muxers
        public const uint HB_MUX_MASK_MP4 = 0x030000;
        public const uint HB_MUX_MASK_MKV = 0x300000;
        public const uint HB_MUX_MASK_WEBM = 0x400000;

        public const uint HB_MUX_AV_MP4 = 0x020000;
        public const uint HB_MUX_AV_MKV = 0x200000;
        public const uint HB_MUX_AV_WEBM = 0x400000;

        // Video Decoders
        public const uint HB_DECODE_SUPPORT_SW = 0x01;
        public const uint HB_DECODE_SUPPORT_QSV = 0x02;
        public const uint HB_DECODE_SUPPORT_NVDEC = 0x04;
        public const uint HB_DECODE_SUPPORT_VIDEOTOOLBOX = 0x08;
        public const uint HB_DECODE_SUPPORT_MF = 0x10;

        public const uint HB_DECODE_SUPPORT_HWACCEL = (HB_DECODE_SUPPORT_NVDEC | HB_DECODE_SUPPORT_VIDEOTOOLBOX | HB_DECODE_SUPPORT_MF);
    }
}
