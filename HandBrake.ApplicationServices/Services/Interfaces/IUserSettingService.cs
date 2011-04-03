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
        /// Get an Integer type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        int GetUserSettingInt(string name);

        /// <summary>
        /// Get an String type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        string GetUserSettingString(string name);

        /// <summary>
        /// Get an Boolean type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        bool GetUserSettingBoolean(string name);

        /// <summary>
        /// Get an Double type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        double GetUserSettingDouble(string name);
    }
}