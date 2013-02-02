// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UserSettingService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The User Setting Serivce
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Specialized;
    using System.IO;
    using System.Linq;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Collections;
    using HandBrake.ApplicationServices.EventArgs;
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
        }

        /// <summary>
        /// The setting changed.
        /// </summary>
        public event SettingEventHandler SettingChanged;

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

            this.OnSettingChanged(new SettingChangedEventArgs {Key = name, Value = value});
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
        /// The on setting changed.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected virtual void OnSettingChanged(SettingChangedEventArgs e)
        {
            SettingEventHandler handler = this.SettingChanged;
            if (handler != null)
            {
                handler(this, e);
            }
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

                    throw new GeneralApplicationException("Warning, your settings have been reset!", "Your user settings file was corrupted or inaccessible. Settings have been reset to defaults.", exc);
                }
                catch (Exception)
                {
                    throw new GeneralApplicationException("Unable to load user settings.", "Your user settings file appears to be inaccessible or corrupted. You may have to delete the file and let HandBrake generate a new one.", exc);
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
