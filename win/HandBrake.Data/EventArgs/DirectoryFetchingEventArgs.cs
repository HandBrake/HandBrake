// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DirectoryFetchingEventArgs.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Event for Getting a Directory Path, so that it can be overriden
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.EventArgs
{
    using System;

    /// <summary>
    /// Event for Getting a Directory Path, so that it can be overriden.
    /// </summary>
    public class DirectoryFetchingEventArgs : EventArgs
    {
        /// <summary>
        /// Gets or sets the directory to use.
        /// </summary>
        public string Directory { get; set; }
    }
}