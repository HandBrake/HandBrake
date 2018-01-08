// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ICopyService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Functions related to copying information.
// </summary>
// --------------------------------------------------------------------------------------------------------------------
namespace HandBrake.Utilities.Interfaces
{
    /// <summary>
    /// Functions related to copying information.
    /// </summary>
    public interface ICopyService
    {
        /// <summary>
        /// Copies text to the Clipboard.
        /// </summary>
        /// <param name="text">Text to copy.</param>
        void Copy(string text);
    }
}