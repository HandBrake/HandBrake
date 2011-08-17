/*  UserSettingService.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

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
                if (File.Exists(this.settingsFile))
                {
                    using (StreamReader reader = new StreamReader(this.settingsFile))
                    {
                        SerializableDictionary<string, object> data = (SerializableDictionary<string, object>)serializer.Deserialize(reader);
                        this.userSettings = data;
                    }
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
        /// This is just a utility method for creating a defaults xml file. Don't use this!!!
        /// </summary>
        private void SetAllDefaults()
        {
            userSettings = new SerializableDictionary<string, object>();
            userSettings[ASUserSettingConstants.X264Step] = 0.25;
            userSettings[ASUserSettingConstants.Verbosity] = 1;
            userSettings[ASUserSettingConstants.WhenCompleteAction] = "Do Nothing";
            userSettings[ASUserSettingConstants.GrowlEncode] = false;
            userSettings[ASUserSettingConstants.GrowlQueue] = false;
            userSettings[ASUserSettingConstants.ProcessPriority] = "Below Normal";
            userSettings[ASUserSettingConstants.PreventSleep] = true;
            userSettings[ASUserSettingConstants.ShowCLI] = false;
            userSettings[ASUserSettingConstants.SaveLogToCopyDirectory] = false;
            userSettings[ASUserSettingConstants.SaveLogWithVideo] = false;
            userSettings[ASUserSettingConstants.DisableLibDvdNav] = false;
            userSettings[ASUserSettingConstants.SendFile] = false;
            userSettings[ASUserSettingConstants.MinScanDuration] = 10;
            userSettings[ASUserSettingConstants.HandBrakeBuild] = 0;
            userSettings[ASUserSettingConstants.HandBrakeVersion] = string.Empty;
            userSettings["updateStatus"] = true;
            userSettings["tooltipEnable"] = true;
            userSettings["defaultPreset"] = string.Empty;
            userSettings["skipversion"] = 0;
            userSettings["autoNaming"] = true;
            userSettings["autoNamePath"] = string.Empty;
            userSettings["appcast"] = "http://handbrake.fr/appcast.xml";
            userSettings["appcast_unstable"] = "http://handbrake.fr/appcast_unstable.xml";
            userSettings["autoNameFormat"] = "{source}-{title}";
            userSettings["VLC_Path"] = "C:\\Program Files\\VideoLAN\\vlc\\vlc.exe";
            userSettings["MainWindowMinimize"] = true;
            userSettings["QueryEditorTab"] = false;
            userSettings["presetNotification"] = false;
            userSettings["trayIconAlerts"] = true;
            userSettings["lastUpdateCheckDate"] = DateTime.Today;
            userSettings["daysBetweenUpdateCheck"] = 7;
            userSettings["useM4v"] = 0;
            userSettings["PromptOnUnmatchingQueries"] = true;
            userSettings["NativeLanguage"] = "Any";
            userSettings["DubMode"] = 255;
            userSettings["CliExeHash"] = string.Empty;
            userSettings["previewScanCount"] = 10;
            userSettings["clearOldLogs"] = true;
            userSettings["AutoNameTitleCase"] = true;
            userSettings["AutoNameRemoveUnderscore"] = true;
            userSettings["ActivityWindowLastMode"] = 0;
            userSettings["useClosedCaption"] = false;
            userSettings["batchMinDuration"] = 35;
            userSettings["batchMaxDuration"] = 65;
            userSettings["defaultPlayer"] = false;
            userSettings["SelectedLanguages"] = new StringCollection();
            userSettings["DubModeAudio"] = 0;
            userSettings["DubModeSubtitle"] = 0;
            userSettings["addOnlyOneAudioPerLanguage"] = true;
            userSettings["MinTitleLength"] = 10;
        }
    }
}
