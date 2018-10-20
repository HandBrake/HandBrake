//
//  HBLocalizationUtilities.h
//  HandBrakeKit
//
//  Created by Damiano Galassi on 20/10/2018.
//

#ifndef HBLocalizationUtilities_h
#define HBLocalizationUtilities_h

#define HBKitLocalizedString(key, comment) \
[[NSBundle bundleForClass:self.class] localizedStringForKey:(key) value:@"" table:nil]


#endif /* HBLocalizationUtilities_h */
