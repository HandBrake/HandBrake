/*  IEncode.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;
    using System.Diagnostics;

    /// <summary>
    /// The IEncode Interface
    /// </summary>
    public interface IEncode
    {
        /// <summary>
        /// Fires when a new CLI Job starts
        /// </summary>
        event EventHandler EncodeStarted;

        /// <summary>
        /// Fires when a CLI job finishes.
        /// </summary>
        event EventHandler EncodeEnded;

        /// <summary>
        /// Gets or sets The HB Process
        /// </summary>
        Process HbProcess { get; set; }

        /// <summary>
        /// Gets a value indicating whether IsEncoding.
        /// </summary>
        bool IsEncoding { get; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        string ActivityLog { get; }

        /// <summary>
        /// Create a preview sample video
        /// </summary>
        /// <param name="query">
        /// The CLI Query
        /// </param>
        void CreatePreviewSample(string query);

        /// <summary>
        /// Kill the CLI process
        /// </summary>
        void Stop();

        /// <summary>
        /// Attempt to Safely kill a DirectRun() CLI
        /// NOTE: This will not work with a MinGW CLI
        /// Note: http://www.cygwin.com/ml/cygwin/2006-03/msg00330.html
        /// </summary>
        void SafelyClose();
    }
}