// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AdvancedEncoderOptionsCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Command for resetting the video / advnaced tabs encoder options.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using Caliburn.Micro;

    using HandBrakeWPF.Commands.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// A Command for resetting the video / advnaced tabs encoder options.
    /// </summary>
    public class AdvancedEncoderOptionsCommand : IAdvancedEncoderOptionsCommand
    {
        /// <summary>
        /// Clear out the advanced options
        /// </summary>
        public void ExecuteClearAdvanced()
        {
            IAdvancedViewModel advancedViewModel = IoC.Get<IAdvancedViewModel>();
            advancedViewModel.Clear();
        }

        /// <summary>
        /// Clear the advanced encoder options out on the video tab.
        /// </summary>
        public void ExecuteClearVideo()
        {
            IVideoViewModel videoViewModel = IoC.Get<IVideoViewModel>();
            videoViewModel.ClearAdvancedSettings();
        }
    }
}
