/*  UserSettingService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

using System.Linq;

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.IO;
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
                this.LoadDefaults();
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

        /// <summary>
        /// Load the User Settings
        /// </summary>
        private void Load()
        {
            try
            {
                // Load up the users current settings file.
                if (File.Exists(this.settingsFile))
                {
                    using (StreamReader reader = new StreamReader(this.settingsFile))
                    {
                        SerializableDictionary<string, object> data = (SerializableDictionary<string, object>)serializer.Deserialize(reader);
                        this.userSettings = data;
                    }
                }
                else
                {
                    this.userSettings = new SerializableDictionary<string, object>();
                }

                // Add any missing / new settings
                SerializableDictionary<string, object> defaults = this.GetDefaults();
                foreach (var item in defaults.Where(item => !this.userSettings.Keys.Contains(item.Key)))
                {
                    this.userSettings.Add(item.Key, item.Value);
                    this.Save();
                }
            }
            catch (Exception exc)
            {
                try
                {
                    this.userSettings = this.GetDefaults();
                    if (File.Exists(this.settingsFile))
                    {
                        File.Delete(this.settingsFile);
                    }
                    this.Save();
                }
                catch (Exception)
                {
                }

                throw;
            }
        }

        /// <summary>
        /// Load Default Settings
        /// </summary>
        private void LoadDefaults()
        {
            string defaults = Path.Combine(Application.StartupPath, "defaultsettings.xml");
            if (File.Exists(defaults))
            {
                using (StreamReader reader = new StreamReader(defaults))
                {
                    SerializableDictionary<string, object> data = (SerializableDictionary<string, object>)serializer.Deserialize(reader);
                    this.userSettings = data;
                }
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
            if (File.Exists("defaultsettings.xml"))
            {
                using (StreamReader reader = new StreamReader("defaultsettings.xml"))
                {
                    return (SerializableDictionary<string, object>)serializer.Deserialize(reader);
                }
            }
            return new SerializableDictionary<string, object>();
        }
    }
}
