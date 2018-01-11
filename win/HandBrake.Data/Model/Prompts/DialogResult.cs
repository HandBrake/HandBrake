// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DialogResult.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Dialog Result;
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Model.Prompts
{
    public enum DialogResult
    {
        None = 0,
        OK = 1,
        Cancel = 2,
        Yes = 6,
        No = 7
    }
}