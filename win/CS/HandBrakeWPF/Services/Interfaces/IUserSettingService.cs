// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IUserSettingService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The User Setting Service Interface.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    using System;

    using SettingChangedEventArgs = HandBrakeWPF.EventArgs.SettingChangedEventArgs;

    /// <summary>
    /// The setting event handler.
    /// </summary>
    /// <param name="sender">
    /// The sender.
    /// </param>
    /// <param name="e">
    /// The e.
    /// </param>
    public delegate void SettingEventHandler(object sender, SettingChangedEventArgs e);

    /// <summary>
    /// The User Setting Service Interface.
    /// </summary>
    public interface IUserSettingService
    {
        /// <summary>
        /// The setting changed.
        /// </summary>
        event SettingEventHandler SettingChanged;

        /// <summary>
        /// Set the specified user setting.
        /// </summary>
        /// <param name="name">
        /// Name of the property
        /// </param>
        /// <param name="value">
        /// The value to store.
        /// </param>
        void SetUserSetting(string name, object value);

        /// <summary>
        /// Get user setting for a given key.
        /// </summary>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <typeparam name="T">
        /// The Type of the setting
        /// </typeparam>
        /// <returns>
        /// The user setting
        /// </returns>
        T GetUserSetting<T>(string name);
    }
}