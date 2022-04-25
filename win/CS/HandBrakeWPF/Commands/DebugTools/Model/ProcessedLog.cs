// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ProcessedLog.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the BrakeBench source code - It may be used under the terms of the 3-Clause BSD License
// </copyright>
// <summary>
//   Defines the ProcessedLog type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.DebugTools.Model
{
    public class ProcessedLog
    {
        public ProcessedLog()
        {
        }

        public decimal? VideoAvgBitrate { get; set; }
    }
}