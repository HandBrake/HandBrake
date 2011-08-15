/*  IUserSettingService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */


namespace HandBrake.ApplicationServices.Services.Interfaces
{
    /// <summary>
    /// The User Setting Service Interace.
    /// </summary>
    public interface IUserSettingService
    {
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

        /// <summary>
        /// Get an StringCollection type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        System.Collections.Specialized.StringCollection GetUserSettingStringCollection(string name);
    }
}