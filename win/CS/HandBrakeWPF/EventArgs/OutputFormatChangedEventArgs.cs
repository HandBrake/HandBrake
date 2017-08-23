// --------------------------------------------------------------------------------------------------------------------
// <copyright file="OutputFormatChangedEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   OutputFormat Changed Event Args
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.EventArgs
{
    using EventArgs = System.EventArgs;

    public class OutputFormatChangedEventArgs : EventArgs
    {
        public OutputFormatChangedEventArgs(string extension)
        {
            this.Extension = extension;
        }

        public string Extension { get; private set; }
    }
}
