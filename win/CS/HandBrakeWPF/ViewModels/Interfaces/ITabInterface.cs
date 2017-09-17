// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ITabInterface.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ITabInterface type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using System;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// Common interface for all the main tab panels
    /// </summary>
    public interface ITabInterface
    {
        event EventHandler<TabStatusEventArgs> TabStatusChanged;

        /// <summary>
        /// Setup the window after a scan.
        /// </summary>
        /// <param name="source">
        /// The source.
        /// </param>
        /// <param name="selectedTitle">
        /// The selected title.
        /// </param>
        /// <param name="currentPreset">
        /// The Current preset
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        void SetSource(Source source, Title selectedTitle, Preset currentPreset, EncodeTask task);

        /// <summary>
        /// Set the selected preset
        /// </summary>
        /// <param name="preset">
        /// The preset.
        /// </param>
        /// <param name="task">
        /// The task.
        /// </param>
        void SetPreset(Preset preset, EncodeTask task);

        /// <summary>
        /// Update all the UI controls based on the encode task passed in.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        void UpdateTask(EncodeTask task);

        bool MatchesPreset(Preset preset);
    }
}
