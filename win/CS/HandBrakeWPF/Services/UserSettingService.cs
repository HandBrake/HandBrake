// --------------------------------------------------------------------------------------------------------------------
// <copyright file="UserSettingService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The User Setting Service
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Xml.Serialization;

    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;

    using GeneralApplicationException = HandBrakeWPF.Exceptions.GeneralApplicationException;
    using SettingChangedEventArgs = HandBrakeWPF.EventArgs.SettingChangedEventArgs;

    /// <summary>
    /// The User Setting Service
    /// </summary>
    public class UserSettingService : IUserSettingService
    {
        /// <summary>
        /// The Settings File
        /// </summary>
        private readonly string settingsFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly()), "settings.xml");

        /// <summary>
        /// The XML Serializer 
        /// </summary>
        private readonly XmlSerializer serializer = new XmlSerializer(typeof(Collections.SerializableDictionary<string, object>));

        /// <summary>
        /// The User Settings
        /// </summary>
        private Collections.SerializableDictionary<string, object> userSettings;

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

            this.OnSettingChanged(new SettingChangedEventArgs { Key = name, Value = value });
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
                    this.serializer.Serialize(strm, this.userSettings);
                }
            }
            catch (Exception exc)
            {
                throw new GeneralApplicationException(
                    Resources.UserSettings_AnErrorOccured,
                    Resources.SettingService_SaveErrorReset,
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
                        Collections.SerializableDictionary<string, object> data = (Collections.SerializableDictionary<string, object>)this.serializer.Deserialize(reader);
                        this.userSettings = data;
                    }
                }
                else if (VersionHelper.IsNightly() && File.Exists(Path.Combine(DirectoryUtilities.GetUserStoragePath(false), "settings.xml")))
                {
                    // Port the release versions config to the nightly.
                    string releasePresetFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(false), "settings.xml");

                    if (!Directory.Exists(DirectoryUtilities.GetUserStoragePath(true)))
                    {
                        Directory.CreateDirectory(DirectoryUtilities.GetUserStoragePath(true));
                    }

                    File.Copy(releasePresetFile, Path.Combine(DirectoryUtilities.GetUserStoragePath(true), "settings.xml"));

                    using (StreamReader reader = new StreamReader(this.settingsFile))
                    {
                        Collections.SerializableDictionary<string, object> data = (Collections.SerializableDictionary<string, object>)this.serializer.Deserialize(reader);
                        this.userSettings = data;
                    }
                }
                else
                {
                    this.userSettings = new Collections.SerializableDictionary<string, object>();
                }

                // Add any missing / new settings
                Collections.SerializableDictionary<string, object> defaults = this.GetDefaults();
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

                    throw new GeneralApplicationException(Resources.UserSettings_YourSettingsHaveBeenReset, Resources.UserSettings_YourSettingsAreCorrupt, exc);
                }
                catch (Exception)
                {
                    throw new GeneralApplicationException(string.Format(Resources.UserSettings_UnableToLoad, this.settingsFile), Resources.UserSettings_UnableToLoadSolution, exc);
                }
            }
        }

        /// <summary>
        /// Load Default Settings
        /// </summary>
        /// <returns>
        /// The get defaults.
        /// </returns>
        private Collections.SerializableDictionary<string, object> GetDefaults()
        {
            try
            {
                Assembly assembly = Assembly.GetEntryAssembly();
                Stream stream = assembly.GetManifestResourceStream("HandBrakeWPF.defaultsettings.xml");
                if (stream != null)
                {
                    return (Collections.SerializableDictionary<string, object>)this.serializer.Deserialize(stream);
                }
            }
            catch (Exception)
            {
                return new Collections.SerializableDictionary<string, object>();
            }

            return new Collections.SerializableDictionary<string, object>();
        }
    }
}
