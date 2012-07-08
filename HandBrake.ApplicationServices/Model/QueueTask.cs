// --------------------------------------------------------------------------------------------------------------------
// <copyright file="QueueTask.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The QueueTask.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model
{
    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// The QueueTask.
    /// </summary>
    public class QueueTask : PropertyChangedBase
    {
        #region Constants and Fields

        /// <summary>
        /// The status.
        /// </summary>
        private QueueItemStatus status;

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets ScannedSource.
        /// </summary>
        public Source ScannedSource { get; set; } 

        /// <summary>
        /// Gets or sets a value indicating whether if this is a user or GUI generated query
        /// </summary>
        public bool CustomQuery { get; set; }

        /// <summary>
        /// Gets or sets Status.
        /// </summary>
        public QueueItemStatus Status
        {
            get
            {
                return this.status;
            }

            set
            {
                this.status = value;
                this.NotifyOfPropertyChange(() => this.Status);
            }
        }

        /// <summary>
        /// Gets or sets the Encode Task.
        /// </summary>
        public EncodeTask Task { get; set; }

        #endregion
    }
}