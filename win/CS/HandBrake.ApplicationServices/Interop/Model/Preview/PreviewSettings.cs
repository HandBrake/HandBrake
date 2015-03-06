// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PreviewSettings.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preview settings.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Preview
{
    using HandBrake.ApplicationServices.Services.Encode.Model;

    /// <summary>
    /// The preview settings.
    /// </summary>
    public class PreviewSettings : EncodeTask
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PreviewSettings"/> class.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        public PreviewSettings(EncodeTask task)
            : base(task)
        {
        }

        // TODO flesh this out into a proper model.
    }
}
