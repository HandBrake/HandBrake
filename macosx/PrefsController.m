#import "PrefsController.h"

@implementation PrefsController

- (void) awakeFromNib
{
    NSUserDefaults * defaults;
    NSDictionary   * appDefaults;
    
    /* Unless the user specified otherwise, default is to check
       for update */
    defaults    = [NSUserDefaults standardUserDefaults];
    appDefaults = [NSDictionary dictionaryWithObject:@"YES"
                   forKey:@"CheckForUpdates"];
    appDefaults = [NSDictionary dictionaryWithObject:@"NO"
                   forKey:@"PixelRatio"];
	appDefaults = [NSDictionary dictionaryWithObject:@"English"
                   forKey:@"DefaultLanguage"];
    [defaults registerDefaults: appDefaults];

    /* Check or uncheck according to the preferences */
    [fUpdateCheck setState: [defaults boolForKey:@"CheckForUpdates"] ?
        NSOnState : NSOffState];
    [fPixelRatio setState: [defaults boolForKey:@"PixelRatio"] ?
        NSOnState : NSOffState];
	// Fill the languages combobox
    [fDefaultLanguage removeAllItems];
	[fDefaultLanguage addItemWithObjectValue: @"Afar"];
	[fDefaultLanguage addItemWithObjectValue: @"Abkhazian"];
	[fDefaultLanguage addItemWithObjectValue: @"Afrikaans"];
	[fDefaultLanguage addItemWithObjectValue: @"Albanian"];
	[fDefaultLanguage addItemWithObjectValue: @"Amharic"];
	[fDefaultLanguage addItemWithObjectValue: @"Arabic"];
	[fDefaultLanguage addItemWithObjectValue: @"Armenian"];
	[fDefaultLanguage addItemWithObjectValue: @"Assamese"];
	[fDefaultLanguage addItemWithObjectValue: @"Avestan"];
	[fDefaultLanguage addItemWithObjectValue: @"Aymara"];
	[fDefaultLanguage addItemWithObjectValue: @"Azerbaijani"];
	[fDefaultLanguage addItemWithObjectValue: @"Bashkir"];
	[fDefaultLanguage addItemWithObjectValue: @"Basque"];
	[fDefaultLanguage addItemWithObjectValue: @"Belarusian"];
	[fDefaultLanguage addItemWithObjectValue: @"Bengali"];
	[fDefaultLanguage addItemWithObjectValue: @"Bihari"];
	[fDefaultLanguage addItemWithObjectValue: @"Bislama"];
	[fDefaultLanguage addItemWithObjectValue: @"Bosnian"];
	[fDefaultLanguage addItemWithObjectValue: @"Breton"];
	[fDefaultLanguage addItemWithObjectValue: @"Bulgarian"];
	[fDefaultLanguage addItemWithObjectValue: @"Burmese"];
	[fDefaultLanguage addItemWithObjectValue: @"Catalan"];
	[fDefaultLanguage addItemWithObjectValue: @"Chamorro"];
	[fDefaultLanguage addItemWithObjectValue: @"Chechen"];
	[fDefaultLanguage addItemWithObjectValue: @"Chichewa; Nyanja"];
	[fDefaultLanguage addItemWithObjectValue: @"Chinese"];
	[fDefaultLanguage addItemWithObjectValue: @"Church Slavic"];
	[fDefaultLanguage addItemWithObjectValue: @"Chuvash"];
	[fDefaultLanguage addItemWithObjectValue: @"Cornish"];
	[fDefaultLanguage addItemWithObjectValue: @"Corsican"];
	[fDefaultLanguage addItemWithObjectValue: @"Croatian"];
	[fDefaultLanguage addItemWithObjectValue: @"Czech"];
	[fDefaultLanguage addItemWithObjectValue: @"Dansk"];
	[fDefaultLanguage addItemWithObjectValue: @"Deutsch"];
	[fDefaultLanguage addItemWithObjectValue: @"Dzongkha"];
	[fDefaultLanguage addItemWithObjectValue: @"English"];
	[fDefaultLanguage addItemWithObjectValue: @"Espanol"];
	[fDefaultLanguage addItemWithObjectValue: @"Esperanto"];
	[fDefaultLanguage addItemWithObjectValue: @"Estonian"];
	[fDefaultLanguage addItemWithObjectValue: @"Faroese"];
	[fDefaultLanguage addItemWithObjectValue: @"Fijian"];
	[fDefaultLanguage addItemWithObjectValue: @"Francais"];
	[fDefaultLanguage addItemWithObjectValue: @"Frisian"];
	[fDefaultLanguage addItemWithObjectValue: @"Georgian"];
	[fDefaultLanguage addItemWithObjectValue: @"Gaelic (Scots)"];
	[fDefaultLanguage addItemWithObjectValue: @"Gallegan"];
	[fDefaultLanguage addItemWithObjectValue: @"Greek, Modern ()"];
	[fDefaultLanguage addItemWithObjectValue: @"Guarani"];
	[fDefaultLanguage addItemWithObjectValue: @"Gujarati"];
	[fDefaultLanguage addItemWithObjectValue: @"Hebrew"];
	[fDefaultLanguage addItemWithObjectValue: @"Herero"];
	[fDefaultLanguage addItemWithObjectValue: @"Hindi"];
	[fDefaultLanguage addItemWithObjectValue: @"Hiri Motu"];
	[fDefaultLanguage addItemWithObjectValue: @"Inuktitut"];
	[fDefaultLanguage addItemWithObjectValue: @"Interlingue"];
	[fDefaultLanguage addItemWithObjectValue: @"Interlingua"];
	[fDefaultLanguage addItemWithObjectValue: @"Indonesian"];
	[fDefaultLanguage addItemWithObjectValue: @"Inupiaq"];
	[fDefaultLanguage addItemWithObjectValue: @"Irish"];
	[fDefaultLanguage addItemWithObjectValue: @"Islenska"];
	[fDefaultLanguage addItemWithObjectValue: @"Italian"];
	[fDefaultLanguage addItemWithObjectValue: @"Javanese"];
	[fDefaultLanguage addItemWithObjectValue: @"Japanese"];
	[fDefaultLanguage addItemWithObjectValue: @"Kalaallisut (Greenlandic)"];
	[fDefaultLanguage addItemWithObjectValue: @"Kannada"];
	[fDefaultLanguage addItemWithObjectValue: @"Kashmiri"];
	[fDefaultLanguage addItemWithObjectValue: @"Kazakh"];
	[fDefaultLanguage addItemWithObjectValue: @"Khmer"];
	[fDefaultLanguage addItemWithObjectValue: @"Kikuyu"];
	[fDefaultLanguage addItemWithObjectValue: @"Kinyarwanda"];
	[fDefaultLanguage addItemWithObjectValue: @"Kirghiz"];
	[fDefaultLanguage addItemWithObjectValue: @"Komi"];
	[fDefaultLanguage addItemWithObjectValue: @"Korean"];
	[fDefaultLanguage addItemWithObjectValue: @"Kuanyama"];
	[fDefaultLanguage addItemWithObjectValue: @"Kurdish"];
	[fDefaultLanguage addItemWithObjectValue: @"Lao"];
	[fDefaultLanguage addItemWithObjectValue: @"Latin"];
	[fDefaultLanguage addItemWithObjectValue: @"Latvian"];
	[fDefaultLanguage addItemWithObjectValue: @"Lingala"];
	[fDefaultLanguage addItemWithObjectValue: @"Lithuanian"];
	[fDefaultLanguage addItemWithObjectValue: @"Letzeburgesch"];
	[fDefaultLanguage addItemWithObjectValue: @"Macedonian"];
	[fDefaultLanguage addItemWithObjectValue: @"Magyar"];
	[fDefaultLanguage addItemWithObjectValue: @"Malay"];
	[fDefaultLanguage addItemWithObjectValue: @"Malayalam"];
	[fDefaultLanguage addItemWithObjectValue: @"Malagasy"];
	[fDefaultLanguage addItemWithObjectValue: @"Maltese"];
	[fDefaultLanguage addItemWithObjectValue: @"Manx"];
	[fDefaultLanguage addItemWithObjectValue: @"Maori"];
	[fDefaultLanguage addItemWithObjectValue: @"Marathi"];
	[fDefaultLanguage addItemWithObjectValue: @"Marshall"];
	[fDefaultLanguage addItemWithObjectValue: @"Moldavian"];
	[fDefaultLanguage addItemWithObjectValue: @"Mongolian"];
	[fDefaultLanguage addItemWithObjectValue: @"Nauru"];
	[fDefaultLanguage addItemWithObjectValue: @"Navajo"];
	[fDefaultLanguage addItemWithObjectValue: @"Ndebele, South"];
	[fDefaultLanguage addItemWithObjectValue: @"Ndebele, North"];
	[fDefaultLanguage addItemWithObjectValue: @"Ndonga"];
	[fDefaultLanguage addItemWithObjectValue: @"Nederlands"];
	[fDefaultLanguage addItemWithObjectValue: @"Nepali"];
	[fDefaultLanguage addItemWithObjectValue: @"Northern Sami"];
	[fDefaultLanguage addItemWithObjectValue: @"Norwegian"];
	[fDefaultLanguage addItemWithObjectValue: @"Norwegian Bokmal"];
	[fDefaultLanguage addItemWithObjectValue: @"Norwegian Nynorsk"];
	[fDefaultLanguage addItemWithObjectValue: @"Occitan (post 1500); Provencal"];
	[fDefaultLanguage addItemWithObjectValue: @"Oriya"];
	[fDefaultLanguage addItemWithObjectValue: @"Oromo"];
	[fDefaultLanguage addItemWithObjectValue: @"Ossetian; Ossetic"];
	[fDefaultLanguage addItemWithObjectValue: @"Panjabi"];
	[fDefaultLanguage addItemWithObjectValue: @"Persian"];
	[fDefaultLanguage addItemWithObjectValue: @"Pali"];
	[fDefaultLanguage addItemWithObjectValue: @"Polish"];
	[fDefaultLanguage addItemWithObjectValue: @"Portugues"];
	[fDefaultLanguage addItemWithObjectValue: @"Pushto"];
	[fDefaultLanguage addItemWithObjectValue: @"Quechua"];
	[fDefaultLanguage addItemWithObjectValue: @"Raeto-Romance"];
	[fDefaultLanguage addItemWithObjectValue: @"Romanian"];
	[fDefaultLanguage addItemWithObjectValue: @"Rundi"];
	[fDefaultLanguage addItemWithObjectValue: @"Russian"];
	[fDefaultLanguage addItemWithObjectValue: @"Sango"];
	[fDefaultLanguage addItemWithObjectValue: @"Sanskrit"];
	[fDefaultLanguage addItemWithObjectValue: @"Sardinian"];
	[fDefaultLanguage addItemWithObjectValue: @"Serbian"];
	[fDefaultLanguage addItemWithObjectValue: @"Sinhalese"];
	[fDefaultLanguage addItemWithObjectValue: @"Slovak"];
	[fDefaultLanguage addItemWithObjectValue: @"Slovenian"];
	[fDefaultLanguage addItemWithObjectValue: @"Samoan"];
	[fDefaultLanguage addItemWithObjectValue: @"Shona"];
	[fDefaultLanguage addItemWithObjectValue: @"Sindhi"];
	[fDefaultLanguage addItemWithObjectValue: @"Somali"];
	[fDefaultLanguage addItemWithObjectValue: @"Sotho, Southern"];
	[fDefaultLanguage addItemWithObjectValue: @"Sundanese"];
	[fDefaultLanguage addItemWithObjectValue: @"Suomi"];
	[fDefaultLanguage addItemWithObjectValue: @"Svenska"];
	[fDefaultLanguage addItemWithObjectValue: @"Swahili"];
	[fDefaultLanguage addItemWithObjectValue: @"Swati"];
	[fDefaultLanguage addItemWithObjectValue: @"Tahitian"];
	[fDefaultLanguage addItemWithObjectValue: @"Tamil"];
	[fDefaultLanguage addItemWithObjectValue: @"Tatar"];
	[fDefaultLanguage addItemWithObjectValue: @"Telugu"];
	[fDefaultLanguage addItemWithObjectValue: @"Tajik"];
	[fDefaultLanguage addItemWithObjectValue: @"Tagalog"];
	[fDefaultLanguage addItemWithObjectValue: @"Thai"];
	[fDefaultLanguage addItemWithObjectValue: @"Tibetan"];
	[fDefaultLanguage addItemWithObjectValue: @"Tigrinya"];
	[fDefaultLanguage addItemWithObjectValue: @"Tonga (Tonga Islands)"];
	[fDefaultLanguage addItemWithObjectValue: @"Tswana"];
	[fDefaultLanguage addItemWithObjectValue: @"Tsonga"];
	[fDefaultLanguage addItemWithObjectValue: @"Turkish"];
	[fDefaultLanguage addItemWithObjectValue: @"Turkmen"];
	[fDefaultLanguage addItemWithObjectValue: @"Twi"];
	[fDefaultLanguage addItemWithObjectValue: @"Uighur"];
	[fDefaultLanguage addItemWithObjectValue: @"Ukrainian"];
	[fDefaultLanguage addItemWithObjectValue: @"Urdu"];
	[fDefaultLanguage addItemWithObjectValue: @"Uzbek"];
	[fDefaultLanguage addItemWithObjectValue: @"Vietnamese"];
	[fDefaultLanguage addItemWithObjectValue: @"Volapk"];
	[fDefaultLanguage addItemWithObjectValue: @"Welsh"];
	[fDefaultLanguage addItemWithObjectValue: @"Wolof"];
	[fDefaultLanguage addItemWithObjectValue: @"Xhosa"];
	[fDefaultLanguage addItemWithObjectValue: @"Yiddish"];
	[fDefaultLanguage addItemWithObjectValue: @"Yoruba"];
	[fDefaultLanguage addItemWithObjectValue: @"Zhuang"];
	[fDefaultLanguage addItemWithObjectValue: @"Zulu"];
	
	[fDefaultLanguage setStringValue:[defaults stringForKey:@"DefaultLanguage"]];
    [fDefaultLanguage selectItemWithObjectValue:[defaults stringForKey:@"DefaultLanguage"]];

}


- (IBAction) OpenPanel: (id) sender;
{
    [NSApp runModalForWindow: fPanel];
}

- (IBAction) ClosePanel: (id) sender;
{
    [NSApp stopModal];
    [fPanel orderOut: sender];
}

- (IBAction) CheckChanged: (id) sender
{
    NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
    
    if( [fUpdateCheck state] == NSOnState )
    {
        [defaults setObject:@"YES" forKey:@"CheckForUpdates"];
    }
    else
    {
        [defaults setObject:@"NO" forKey:@"CheckForUpdates"];
    }
    
	if( [fPixelRatio state] == NSOnState )
    {
        [defaults setObject:@"YES" forKey:@"PixelRatio"];
    }
    else
    {
        [defaults setObject:@"NO" forKey:@"PixelRatio"];
    }
	
	[defaults setObject:[fDefaultLanguage objectValueOfSelectedItem]  forKey:@"DefaultLanguage"];
}

@end
