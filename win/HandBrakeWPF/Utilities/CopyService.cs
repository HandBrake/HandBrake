// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CopyService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.Windows;
    using HandBrake.Utilities.Interfaces;

    public class CopyService : ICopyService
    {
        public void Copy(string text)
        {
            Clipboard.SetDataObject(text, true);
        }
    }
}