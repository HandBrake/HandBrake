// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DialogService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Utilities
{
    using HandBrake.Model.Prompts;
    using HandBrake.Utilities.Interfaces;

    public class DialogService : IDialogService
    {
        public DialogResult Show(string message, string header, DialogButtonType buttons, DialogType type)
        {
            throw new System.NotImplementedException();
        }

        public DialogResult Show(string message)
        {
            throw new System.NotImplementedException();
        }
    }
}