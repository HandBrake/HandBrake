/* HBAdvancedController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@interface HBAdvancedController : NSObject
{
    /* Advanced Tab for opts fX264optView*/
    NSBox                       * fOptionsBox;
    
    IBOutlet NSView             * fX264optView;
    IBOutlet NSView             * fEmptyView;
    IBOutlet NSTextField        * fX264optViewTitleLabel;
    IBOutlet NSTextField        * fDisplayX264OptionsLabel;
    IBOutlet NSTextField        * fDisplayX264Options;
    IBOutlet NSTextField        * fX264optBframesLabel;
    IBOutlet NSPopUpButton      * fX264optBframesPopUp;
    IBOutlet NSTextField        * fX264optRefLabel;
    IBOutlet NSPopUpButton      * fX264optRefPopUp;
    IBOutlet NSTextField        * fX264optNfpskipLabel;
    IBOutlet NSButton           * fX264optNfpskipSwitch;
    IBOutlet NSTextField        * fX264optNodctdcmtLabel;
    IBOutlet NSButton           * fX264optNodctdcmtSwitch;
    IBOutlet NSTextField        * fX264optSubmeLabel;
    IBOutlet NSPopUpButton      * fX264optSubmePopUp;
    IBOutlet NSTextField        * fX264optTrellisLabel;
    IBOutlet NSPopUpButton      * fX264optTrellisPopUp;
    IBOutlet NSTextField        * fX264optMixedRefsLabel;
    IBOutlet NSButton           * fX264optMixedRefsSwitch;
    IBOutlet NSTextField        * fX264optMotionEstLabel;
    IBOutlet NSPopUpButton      * fX264optMotionEstPopUp;
    IBOutlet NSTextField        * fX264optMERangeLabel;
    IBOutlet NSPopUpButton      * fX264optMERangePopUp;
    IBOutlet NSTextField        * fX264optWeightBLabel;
    IBOutlet NSButton           * fX264optWeightBSwitch;
    IBOutlet NSTextField        * fX264optBRDOLabel;
    IBOutlet NSButton           * fX264optBRDOSwitch;
    IBOutlet NSTextField        * fX264optBPyramidLabel;
    IBOutlet NSButton           * fX264optBPyramidSwitch;
    IBOutlet NSTextField        * fX264optBiMELabel;
    IBOutlet NSButton           * fX264optBiMESwitch;
    IBOutlet NSTextField        * fX264optDirectPredLabel;
    IBOutlet NSPopUpButton      * fX264optDirectPredPopUp;
    IBOutlet NSTextField        * fX264optDeblockLabel;
    IBOutlet NSPopUpButton      * fX264optAlphaDeblockPopUp;
    IBOutlet NSPopUpButton      * fX264optBetaDeblockPopUp;
    IBOutlet NSTextField        * fX264optAnalyseLabel;
    IBOutlet NSPopUpButton      * fX264optAnalysePopUp;
    IBOutlet NSTextField        * fX264opt8x8dctLabel;
    IBOutlet NSButton           * fX264opt8x8dctSwitch;
    IBOutlet NSTextField        * fX264optCabacLabel;
    IBOutlet NSButton           * fX264optCabacSwitch;
}

// x264 Advanced Panel Methods
- (void) setView: (NSBox *) box;
- (BOOL) loadMyNibFile;
- (NSString *) optionsString;
- (void) setOptions: (NSString *)string;
- (void) enableUI: (bool) b;
- (void) setHidden: (BOOL) hide;
- (IBAction) X264AdvancedOptionsAnimate: (id) sender;
- (IBAction) X264AdvancedOptionsSet: (id) sender;
- (IBAction) X264AdvancedOptionsStandardizeOptString: (id) sender;
- (IBAction) X264AdvancedOptionsSetCurrentSettings: (id) sender;
- (NSString *)  X264AdvancedOptionsStandardizeOptNames:(NSString *) cleanOptNameString;
- (IBAction) X264AdvancedOptionsChanged: (id) sender;

@end
