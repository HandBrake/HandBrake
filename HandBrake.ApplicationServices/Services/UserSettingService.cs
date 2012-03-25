/*  UserSettingService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Specialized;
    using System.IO;
    using System.Linq;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Collections;
    using HandBrake.ApplicationServices.Exceptions;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The User Setting Serivce
    /// </summary>
    public class UserSettingService : IUserSettingService
    {
        /// <summary>
        /// The Settings File
        /// </summary>
        private readonly string settingsFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\HandBrake\\settings.xml";

        /// <summary>
        /// The XML Serializer 
        /// </summary>
        readonly XmlSerializer serializer = new XmlSerializer(typeof(SerializableDictionary<string, object>));

        /// <summary>
        /// The User Settings
        /// </summary>
        private SerializableDictionary<string, object> userSettings;

        /// <summary>
        /// Initializes a new instance of the <see cref="UserSettingService"/> class.
        /// </summary>
        public UserSettingService()
        {
            this.Load();
            if (userSettings == null || userSettings.Count == 0)
            {
                this.userSettings = this.GetDefaults();
            }
        }

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
            this.userSettings[name] = value;
            this.Save();
        }

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
        public T GetUserSetting<T>(string name)
        {
            if (this.userSettings.ContainsKey(name))
            {
                return (T)this.userSettings[name];
            }

            return default(T);
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
        public StringCollection GetUserSettingStringCollection(string name)
        {
            return (StringCollection)this.userSettings[name];
        }

        /// <summary>
        /// Save the User Settings
        /// </summary>
        private void Save()
        {
            try
            {
                string directory = Path.GetDirectoryName(this.settingsFile);
                if (!string.IsNullOrEmpty(directory) && !Directory.Exists(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                using (FileStream strm = new FileStream(this.settingsFile, FileMode.Create, FileAccess.Write))
                {
                    serializer.Serialize(strm, this.userSettings);
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    "A problem occured when trying to save your preferences.",
                    "Any settings you changed may need to be reset the next time HandBrake launches.",
                    exc);
            }
        }

        /// <summary>
        /// Load the User Settings
        /// </summary>
        private void Load()
        {
            try
            {
                if (File.Exists(this.settingsFile))
                {
                    using (StreamReader reader = new StreamReader(this.settingsFile))
                    {
                        SerializableDictionary<string, object> data = (SerializableDictionary<string, object>)serializer.Deserialize(reader);
                        this.userSettings = data;

                        // Add any new settings
                        SerializableDictionary<string, object> defaults = this.GetDefaults();
                        foreach (var item in defaults.Where(item => !this.userSettings.Keys.Contains(item.Key)))
                        {
                            this.userSettings.Add(item.Key, item.Value);
                        }
                    }

                    this.Save();
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    "HandBrake has detected corruption in the settings file. User settings will now be reset to defaults.",
                    "Please restart HandBrake before continuing.",
                    exc);
            }
        }

        /// <summary>
        /// Load Default Settings
        /// </summary>
        /// <returns>
        /// The get defaults.
        /// </returns>
        private SerializableDictionary<string, object> GetDefaults()
        {
            try
            {
                string defaults = Path.Combine(Application.StartupPath, "defaultsettings.xml");
                if (File.Exists(defaults))
                {
                    using (StreamReader reader = new StreamReader(defaults))
                    {
                        return (SerializableDictionary<string, object>)serializer.Deserialize(reader);
                    }
                }

                throw new GeneralApplicationException(
                "User default settings file is missing. This install of HandBrake may be corrupted.",
                "Try re-installing HandBrake.",
                null);
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                   "User default settings file is missing or inaccessible. This install of HandBrake may be corrupted.",
                   "Try re-installing HandBrake.",
                   exc);
            }
        }
    }
}
