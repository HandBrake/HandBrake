﻿// --------------------------------------------------------------------------------------------------------------------
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
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.IO;
    using System.Linq;

    using HandBrake.Interop.Model;
    using HandBrake.Interop.Utilities;

    using HandBrakeWPF.Extensions;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Utilities;

    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    using GeneralApplicationException = Exceptions.GeneralApplicationException;
    using SettingChangedEventArgs = EventArgs.SettingChangedEventArgs;

    /// <summary>
    /// The User Setting Service
    /// </summary>
    public class UserSettingService : IUserSettingService
    {
        private readonly string settingsFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(VersionHelper.IsNightly()), "settings.json");
        private readonly string releaseSettingsFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(false), "settings.json");
        private readonly string nightlySettingsFile = Path.Combine(DirectoryUtilities.GetUserStoragePath(true), "settings.json");
        private readonly JsonSerializerSettings settings = new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore };
        private Dictionary<string, object> userSettings;

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
                if (typeof(T) == typeof(int))
                {
                    object storedValue = this.userSettings[name];
                    object converted = storedValue?.ToString().ToInt();
                    return (T)converted;
                }

                // Treat String Arrays as StringCollections.  TODO refactor upstream code to more traditional string arrays.
                object settingValue = this.userSettings[name];
                if (settingValue != null && settingValue.GetType() == typeof(JArray))
                {
                    string[] stringArr = ((JArray)settingValue).ToObject<string[]>();
                    StringCollection stringCollection = new StringCollection();
                    foreach (var item in stringArr)
                    {
                        stringCollection.Add(item);
                    }

                    settingValue = stringCollection;
                }

                return (T)settingValue;
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

                using (StreamWriter file = new StreamWriter(new FileStream(this.settingsFile, FileMode.Create, FileAccess.Write)))
                {
                    string appSettings = JsonConvert.SerializeObject(this.userSettings, Formatting.Indented, this.settings);
                    file.Write(appSettings);
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
                        string appSettings = reader.ReadToEnd();
                        Dictionary<string, object> deserialisedSettings = JsonConvert.DeserializeObject<Dictionary<string, object>>(appSettings);

                        this.userSettings = deserialisedSettings;
                    }
                }
                else if (VersionHelper.IsNightly() && File.Exists(this.releaseSettingsFile))
                {
                    // Port the release versions config to the nightly.
                    if (!Directory.Exists(DirectoryUtilities.GetUserStoragePath(true)))
                    {
                        Directory.CreateDirectory(DirectoryUtilities.GetUserStoragePath(true));
                    }

                    File.Copy(this.releaseSettingsFile, this.nightlySettingsFile);

                    using (StreamReader reader = new StreamReader(this.settingsFile))
                    {
                        string appSettings = reader.ReadToEnd();
                        Dictionary<string, object> deserialisedSettings = JsonConvert.DeserializeObject<Dictionary<string, object>>(appSettings);
                        this.userSettings = deserialisedSettings;
                    }
                }
                else
                {
                    this.userSettings = new Dictionary<string, object>();
                }

                // Add any missing / new settings
                Dictionary<string, object> defaults = this.GetDefaults();
                foreach (var item in defaults.Where(item => !this.userSettings.Keys.Contains(item.Key)))
                {
                    this.userSettings.Add(item.Key, item.Value);
                    this.Save();
                }

                // Legacy Settings forced Reset.
                this.userSettings[UserSettingConstants.ScalingMode] = VideoScaler.Lanczos;
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
        private Dictionary<string, object> GetDefaults()
        {
            Dictionary<string, object> defaults = new Dictionary<string, object>();

            defaults.Add(UserSettingConstants.Verbosity, 1);

            // General
            defaults.Add(UserSettingConstants.UpdateStatus, true);
            defaults.Add(UserSettingConstants.LastUpdateCheckDate, DateTime.Now.Date.AddDays(-30));
            defaults.Add(UserSettingConstants.DaysBetweenUpdateCheck, 1);
            defaults.Add(UserSettingConstants.UseDarkTheme, false);
            defaults.Add(UserSettingConstants.ShowPreviewOnSummaryTab, true);
            defaults.Add(UserSettingConstants.MainWindowMinimize, false);
            defaults.Add(UserSettingConstants.ClearCompletedFromQueue, false);
            defaults.Add(UserSettingConstants.ShowStatusInTitleBar, false);
            defaults.Add(UserSettingConstants.ShowAddAllToQueue, false);
            defaults.Add(UserSettingConstants.ShowAddSelectionToQueue, false);
            defaults.Add(UserSettingConstants.VLCPath, @"C:\Program Files\VideoLAN\vlc\vlc.exe");

            // Output Files
            defaults.Add(UserSettingConstants.AutoNaming, true);
            defaults.Add(UserSettingConstants.AutoNamePath, string.Empty);
            defaults.Add(UserSettingConstants.AutoNameFormat, "{source}-{title}");
            defaults.Add(UserSettingConstants.AutonameFilePrePostString, "output_");
            defaults.Add(UserSettingConstants.AutoNameTitleCase, true);
            defaults.Add(UserSettingConstants.AutoNameRemoveUnderscore, true);
            defaults.Add(UserSettingConstants.AutonameFileCollisionBehaviour, 0);
            defaults.Add(UserSettingConstants.AlwaysUseDefaultPath, true);
            defaults.Add(UserSettingConstants.RemovePunctuation, false);
            defaults.Add(UserSettingConstants.FileOverwriteBehaviour, 0);
            defaults.Add(UserSettingConstants.UseM4v, 0);

            // When Done
            defaults.Add(UserSettingConstants.SendFile, false);
            defaults.Add(UserSettingConstants.WhenCompleteAction, 0);
            defaults.Add(UserSettingConstants.WhenDonePerformActionImmediately, false);
            defaults.Add(UserSettingConstants.PlaySoundWhenDone, false);
            defaults.Add(UserSettingConstants.PlaySoundWhenQueueDone, false);
            defaults.Add(UserSettingConstants.WhenDoneAudioFile, string.Empty);

            // Video
            defaults.Add(UserSettingConstants.EnableQuickSyncEncoding, true);
            defaults.Add(UserSettingConstants.EnableQuickSyncDecoding, true);
            defaults.Add(UserSettingConstants.UseQSVDecodeForNonQSVEnc, false);
            defaults.Add(UserSettingConstants.EnableVceEncoder, true);
            defaults.Add(UserSettingConstants.EnableNvencEncoder, true);

            // Advanced
            defaults.Add(UserSettingConstants.PreventSleep, true);
            defaults.Add(UserSettingConstants.PauseEncodingOnLowBattery, true);
            defaults.Add(UserSettingConstants.LowBatteryLevel, 15);

            defaults.Add(UserSettingConstants.DisableLibDvdNav, false);
            defaults.Add(UserSettingConstants.PauseOnLowDiskspace, true);
            defaults.Add(UserSettingConstants.PauseQueueOnLowDiskspaceLevel, 2000000000L);
            defaults.Add(UserSettingConstants.PauseEncodeOnLowDiskspaceLevel, 2000000000L);
            defaults.Add(UserSettingConstants.PreviewScanCount, 10);
            defaults.Add(UserSettingConstants.MinScanDuration, 10);
            defaults.Add(UserSettingConstants.ProcessPriorityInt, 3);
            defaults.Add(UserSettingConstants.X264Step, 0.5);
            defaults.Add(UserSettingConstants.SaveLogToCopyDirectory, false);
            defaults.Add(UserSettingConstants.SaveLogWithVideo, false);
            defaults.Add(UserSettingConstants.ClearOldLogs, true);

            // Preview
            defaults.Add(UserSettingConstants.PreviewRotationFlip, false);
            defaults.Add(UserSettingConstants.LastPreviewDuration, 30);
            defaults.Add(UserSettingConstants.DefaultPlayer, false);

            // Experimental
            defaults.Add(UserSettingConstants.RemoteServiceEnabled, false);
            defaults.Add(UserSettingConstants.RemoteServicePort, true);

            // Misc
            defaults.Add(UserSettingConstants.ShowPresetPanel, false);
            defaults.Add(UserSettingConstants.ScalingMode, 0);
            defaults.Add(UserSettingConstants.ForcePresetReset, 3);

            return defaults;
        }
    }
}