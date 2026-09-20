namespace HandBrakeWPF.ViewModels
{
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;

    using HandBrake.Interop.Interop;
    using HandBrake.Interop.Interop.Interfaces.Model;
    using HandBrake.Interop.Utilities;

    public class SubtitleLanguageViewModel : ViewModelBase
    {
        public SubtitleLanguageViewModel(IEnumerable<Language> languages, string preferredLanguageCode)
        {
            List<Language> availableLanguages = languages
                .Where(language => language.Code != HandBrakeLanguagesHelper.AnyLanguage.Code)
                .ToList();

            string systemLanguageCode = CultureInfo.CurrentUICulture.ThreeLetterISOLanguageName;
            Language systemLanguage = availableLanguages.FirstOrDefault(language => language.Code == systemLanguageCode);
            Language preferredLanguage = availableLanguages.FirstOrDefault(language => language.Code == preferredLanguageCode);

            this.Languages = availableLanguages
                .OrderByDescending(language => language == systemLanguage)
                .ThenByDescending(language => language == preferredLanguage)
                .ThenBy(language => language.DisplayNative)
                .ToList();
            this.SelectedLanguage = systemLanguage ?? preferredLanguage ?? this.Languages.FirstOrDefault();
        }

        public IEnumerable<Language> Languages { get; }

        public Language SelectedLanguage { get; set; }

        public void Confirm()
        {
            this.TryClose(true);
        }

        public void Cancel()
        {
            this.TryClose(false);
        }
    }
}
