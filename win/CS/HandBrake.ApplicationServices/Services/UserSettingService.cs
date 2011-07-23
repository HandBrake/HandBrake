/*  UserSettingService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The User Setting Serivce
    /// </summary>
    public class UserSettingService : IUserSettingService
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
        public void SetUserSetting(string name, object value)
        {
            Properties.Settings.Default[name] = value;
            Properties.Settings.Default.Save();
        }

        /// <summary>
        /// Get an Integer type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        public int GetUserSettingInt(string name)
        {
            int value;
            int.TryParse(Properties.Settings.Default[name].ToString(), out value);

            return value;
        }

        /// <summary>
        /// Get an String type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        public string GetUserSettingString(string name)
        {
            return Properties.Settings.Default[name].ToString();
        }

        /// <summary>
        /// Get an Boolean type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        public bool GetUserSettingBoolean(string name)
        {
            bool value;
            bool.TryParse(Properties.Settings.Default[name].ToString(), out value);

            return value;
        }

        /// <summary>
        /// Get an Double type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        public double GetUserSettingDouble(string name)
        {
            double value;
            double.TryParse(Properties.Settings.Default[name].ToString(), out value);

            return value;
        }

        /// <summary>
        /// Get an StringCollection type user setting
        /// </summary>
        /// <param name="name">
        /// The setting name
        /// </param>
        /// <returns>
        /// The settings value
        /// </returns>
        public System.Collections.Specialized.StringCollection GetUserSettingStringCollection(string name)
        {
            System.Collections.Specialized.StringCollection value;

            value = (System.Collections.Specialized.StringCollection) Properties.Settings.Default[name];

            return value;
        }

    }
}
