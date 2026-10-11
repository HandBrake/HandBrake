//
//  SPUAppcastSigningValidationStatus.h
//  Sparkle
//
//  Created on 12/30/25.
//  Copyright © 2025 Sparkle Project. All rights reserved.
//

#ifndef SPUAppcastSigningValidationStatus_h
#define SPUAppcastSigningValidationStatus_h

typedef NS_ENUM(NSInteger, SPUAppcastSigningValidationStatus)
{
    /**
     The bundle does not opt into requiring appcast signing and no validation of the appcast feed is done.
     */
    SPUAppcastSigningValidationStatusSkipped = 0,
    
    /**
     The appcast is signed and validation has passed succesfully.
     */
    SPUAppcastSigningValidationStatusSucceeded,
    
    /**
     The appcast is signed and validation has failed. In this case, appcast items operate in a 'safe' fallback mode
     meaning that they cannot be marked as a critical update, cannot be marked as informational update,
     and will not have any release note or link references.
     */
    SPUAppcastSigningValidationStatusFailed,
};

#endif /* SPUAppcastSigningValidationStatus_h */
