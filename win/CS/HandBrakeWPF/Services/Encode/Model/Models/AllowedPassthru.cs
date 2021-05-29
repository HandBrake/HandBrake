// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AllowedPassthru.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Allowed Passthru Options
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models
{
    using System.Collections.Generic;

    public class AllowedPassthru
    {
        public AllowedPassthru()
        {
            this.AudioAllowAACPass = true;
            this.AudioAllowAC3Pass = true;
            this.AudioAllowDTSHDPass = true;
            this.AudioAllowDTSPass = true;
            this.AudioAllowMP3Pass = true;
            this.AudioAllowEAC3Pass = true;
            this.AudioAllowTrueHDPass = true;
            this.AudioAllowFlacPass = true;
            this.AudioAllowMP2Pass = true;

            this.AudioEncoderFallback = AudioEncoder.Ac3;
        }

        public AllowedPassthru(bool initialValue)
        {
            this.AudioAllowAACPass = initialValue;
            this.AudioAllowAC3Pass = initialValue;
            this.AudioAllowDTSHDPass = initialValue;
            this.AudioAllowDTSPass = initialValue;
            this.AudioAllowMP3Pass = initialValue;
            this.AudioAllowEAC3Pass = initialValue;
            this.AudioAllowTrueHDPass = initialValue;
            this.AudioAllowFlacPass = initialValue;
            this.AudioAllowMP2Pass = initialValue;

            this.AudioEncoderFallback = AudioEncoder.Ac3;
        }


        public AllowedPassthru(AllowedPassthru initialValue)
        {
            this.AudioAllowAACPass = initialValue.AudioAllowAACPass;
            this.AudioAllowAC3Pass = initialValue.AudioAllowAC3Pass;
            this.AudioAllowDTSHDPass = initialValue.AudioAllowDTSHDPass;
            this.AudioAllowDTSPass = initialValue.AudioAllowDTSPass;
            this.AudioAllowMP3Pass = initialValue.AudioAllowMP3Pass;
            this.AudioAllowEAC3Pass = initialValue.AudioAllowEAC3Pass;
            this.AudioAllowTrueHDPass = initialValue.AudioAllowTrueHDPass;
            this.AudioAllowFlacPass = initialValue.AudioAllowFlacPass;
            this.AudioAllowMP2Pass = initialValue.AudioAllowMP2Pass;

            this.AudioEncoderFallback = initialValue.AudioEncoderFallback;
        }


        public bool AudioAllowAACPass { get; set; }

        public bool AudioAllowAC3Pass { get; set; }

        public bool AudioAllowDTSHDPass { get; set; }

        public bool AudioAllowDTSPass { get; set; }

        public bool AudioAllowMP3Pass { get; set; }

        public bool AudioAllowTrueHDPass { get; set; }

        public bool AudioAllowFlacPass { get; set; }

        public bool AudioAllowEAC3Pass { get; set; }

        public bool AudioAllowMP2Pass { get; set; }

        public AudioEncoder AudioEncoderFallback { get; set; }

        public IEnumerable<AudioEncoder> AllowedPassthruOptions
        {
            get
            {
                List<AudioEncoder> audioEncoders = new List<AudioEncoder>();
                if (this.AudioAllowAACPass)
                {
                    audioEncoders.Add(AudioEncoder.AacPassthru);
                }
                if (this.AudioAllowAC3Pass)
                {
                    audioEncoders.Add(AudioEncoder.Ac3Passthrough);
                }
                if (this.AudioAllowDTSHDPass)
                {
                    audioEncoders.Add(AudioEncoder.DtsHDPassthrough);
                }
                if (this.AudioAllowDTSPass)
                {
                    audioEncoders.Add(AudioEncoder.DtsPassthrough);
                }
                if (this.AudioAllowMP3Pass)
                {
                    audioEncoders.Add(AudioEncoder.Mp3Passthru);
                }
                if (this.AudioAllowTrueHDPass)
                {
                    audioEncoders.Add(AudioEncoder.TrueHDPassthrough);
                }
                if (this.AudioAllowFlacPass)
                {
                    audioEncoders.Add(AudioEncoder.FlacPassthru);
                }
                if (this.AudioAllowEAC3Pass)
                {
                    audioEncoders.Add(AudioEncoder.EAc3Passthrough);
                }
                if (this.AudioAllowMP2Pass)
                {
                    audioEncoders.Add(AudioEncoder.Mp2Passthru);
                }

                return audioEncoders;
            }
        }

        public void SetFalse()
        {
            this.AudioAllowAACPass = false;
            this.AudioAllowAC3Pass = false;
            this.AudioAllowDTSHDPass = false;
            this.AudioAllowDTSPass = false;
            this.AudioAllowMP3Pass = false;
            this.AudioAllowEAC3Pass = false;
            this.AudioAllowTrueHDPass = false;
            this.AudioAllowFlacPass = false;
            this.AudioAllowMP2Pass = false;
        }
    }
}