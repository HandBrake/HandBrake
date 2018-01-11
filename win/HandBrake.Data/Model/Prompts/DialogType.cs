// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DialogType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Dialog Type;
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Model.Prompts
{
    public enum DialogType
    {
        //
        // Summary:
        //     No icon is displayed.
        None = 0,

        //
        // Summary:
        //     The message box contains a symbol consisting of white X in a circle with a red
        //     background.
        Error = 16,

        //
        // Summary:
        //     The message box contains a symbol consisting of a question mark in a circle.
        Question = 32,

        //
        // Summary:
        //     The message box contains a symbol consisting of an exclamation point in a triangle
        //     with a yellow background.
        Warning = 48,

        //
        // Summary:
        //     The message box contains a symbol consisting of a lowercase letter i in a circle.
        Information = 64
    }
}