// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAdvancedEncoderOptionsCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The AdvancedEncoderOptionsCommand interface.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.Interfaces
{
    /// <summary>
    /// The AdvancedEncoderOptionsCommand interface.
    /// </summary>
    public interface IAdvancedEncoderOptionsCommand
    {
        /// <summary>
        /// Clear out the advanced options
        /// </summary>
        void ExecuteClearAdvanced();

        /// <summary>
        /// Clear the advanced encoder options out on the video tab.
        /// </summary>
        void ExecuteClearVideo();
    }
}