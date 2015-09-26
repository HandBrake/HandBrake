// --------------------------------------------------------------------------------------------------------------------
// <copyright file="INotifyPropertyChangedEx.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Extends <see cref="T:System.ComponentModel.INotifyPropertyChanged" /> such that the change event can be raised by external parties.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities.Interfaces
{
    using System.ComponentModel;

    /// <summary>
    /// Extends <see cref="T:System.ComponentModel.INotifyPropertyChanged"/> such that the change event can be raised by external parties.
    /// </summary>
    public interface INotifyPropertyChangedEx : INotifyPropertyChanged
    {
        /// <summary>
        /// Enables/Disables property change notification.
        /// </summary>
        bool IsNotifying { get; set; }

        /// <summary>
        /// Notifies subscribers of the property change.
        /// </summary>
        /// <param name="propertyName">
        /// Name of the property.
        /// </param>
        void NotifyOfPropertyChange(string propertyName);

        /// <summary>
        /// Raises a change notification indicating that all bindings should be refreshed.
        /// </summary>
        void Refresh();
    }
}
