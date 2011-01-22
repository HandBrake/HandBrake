/*  VideoEncoderMode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// The Mode of Video Encoding. Bitrate, Filesize or Quality
    /// </summary>
    public enum VideoEncodeMode
    {
        TargetSize = 0,
        AverageBitrate,
        ConstantQuality
    }
}
