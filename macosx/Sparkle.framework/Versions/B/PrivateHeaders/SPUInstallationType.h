//
//  SPUInstallationType.h
//  Sparkle
//
//  Created by Mayur Pawashe on 7/24/16.
//  Copyright © 2016 Sparkle Project. All rights reserved.
//

#ifndef SPUInstallationType_h
#define SPUInstallationType_h

#define SPUInstallationTypeApplication @"application" // the default installation type for ordinary application updates
#define SPUInstallationTypeGuidedPackage @"package" // the preferred installation type for package installations
#define SPUInstallationTypeInteractivePackage @"interactive-package" // removed installation type; use guided package instead

#define SPUInstallationTypesArray (@[SPUInstallationTypeApplication, SPUInstallationTypeGuidedPackage])
#define SPUValidInstallationType(x) ((x != nil) && [SPUInstallationTypesArray containsObject:(NSString * _Nonnull)x])

#endif /* SPUInstallationType_h */
