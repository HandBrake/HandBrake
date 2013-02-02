/* $Id: HBPreviewController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#include "hb.h"
/* Needed for Quicktime movie previews */
#import <QTKit/QTKit.h> 

@class HBController;

@interface PreviewController : NSWindowController <NSWindowDelegate>
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    HBController             * fHBController;     // reference to HBController
    
    NSMutableDictionary      * fPicturePreviews;  // NSImages, one for each preview libhb creates, created lazily
    int                        fPicture;

    CALayer                  * fWhiteBackground;
    CALayer                  * fPictureLayer;
    IBOutlet NSBox           * fPictureControlBox;
    IBOutlet NSBox           * fEncodingControlBox;
    IBOutlet NSBox           * fMoviePlaybackControlBox;

    IBOutlet NSSlider        * fPictureSlider;
    IBOutlet NSTextField     * fInfoField;
    IBOutlet NSTextField     * fscaleInfoField;
    
    CGFloat                  backingScaleFactor;
    
    /* Hud Control Overlay */
    NSTimer                         * fHudTimer;
    int                               hudTimerSeconds;
    
    /* Full Screen Mode Toggle */
    BOOL                              scaleToScreen;
    IBOutlet NSButton               * fScaleToScreenToggleButton;
    IBOutlet NSButton               * fPictureSettingsToggleButton;
    
    /* Movie Previews */
    QTMovie                         * aMovie;
    IBOutlet QTMovieView            * fMovieView;
    /* Playback Panel Controls */
    IBOutlet NSButton               * fPlayPauseButton;
    IBOutlet NSButton               * fGoToBeginningButton;
    IBOutlet NSButton               * fGoToEndButton;
    IBOutlet NSButton               * fGoForwardOneFrameButton;
    IBOutlet NSButton               * fGoBackwardOneFrameButton;
    IBOutlet NSSlider               * fMovieScrubberSlider;
    IBOutlet NSButton               * fGoToStillPreviewButton;
    IBOutlet NSTextField            * fMovieInfoField;
    NSTimer                         * fMovieTimer;
    
    IBOutlet NSButton               * fCreatePreviewMovieButton;
    IBOutlet NSButton               * fCancelPreviewMovieButton;
    IBOutlet NSButton               * fShowPreviewMovieButton;
    NSString                        * fPreviewMoviePath;
    IBOutlet NSProgressIndicator    * fMovieCreationProgressIndicator;
    hb_handle_t                     * fPreviewLibhb;           // private libhb for creating previews
    NSInteger                         fEncodeState;
    NSTimer                         * fLibhbTimer;             // timer for retrieving state from libhb
    IBOutlet NSTextField            * fPreviewMovieStatusField; 
    IBOutlet NSPopUpButton          * fPreviewMovieLengthPopUp; // popup of choices for length of preview in seconds
}
- (id)init;

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void) setHBController: (HBController *)controller;
- (void) displayPreview;

- (IBAction) settingsChanged: (id) sender;
- (IBAction) pictureSliderChanged: (id) sender;
- (IBAction) showPictureSettings:(id)sender;
- (NSString*) pictureSizeInfoString;

- (IBAction) toggleScaleToScreen:(id)sender;

/* HUD overlay */
- (void) enableHudControls;
- (void) disableHudControls;

- (void) startHudTimer;
- (void) stopHudTimer;

/* Movie Previews */
- (void) startReceivingLibhbNotifications;
- (void) stopReceivingLibhbNotifications;

- (void) installMovieCallbacks;
- (void) removeMovieCallbacks;

- (IBAction) cancelCreateMoviePreview: (id) sender;
- (IBAction) createMoviePreview: (id) sender;
- (void) libhbStateChanged: (hb_state_t ) state;
- (IBAction) showMoviePreview: (NSString *) path;
- (IBAction) showPicturesPreview: (id) sender;
- (IBAction) toggleMoviePreviewPlayPause: (id) sender;
- (IBAction) moviePlaybackGoToBeginning: (id) sender;
- (IBAction) moviePlaybackGoToEnd: (id) sender;
- (IBAction) moviePlaybackGoBackwardOneFrame: (id) sender;
- (IBAction) moviePlaybackGoForwardOneFrame: (id) sender;

- (void) initPreviewScrubberForMovie;
- (void) adjustPreviewScrubberForCurrentMovieTime;
- (IBAction) previewScrubberChanged: (id) sender;
- (BOOL) isPlaying;

- (void) startMovieTimer;
- (void) stopMovieTimer;

- (NSString*) SMTPETimecode: (QTTime)time;
- (QTTime)SliderToQTTime:(double)time;

- (IBAction) previewDurationPopUpChanged: (id) sender;

+ (NSImage *) makeImageForPicture: (NSInteger)pictureIndex
                libhb:(hb_handle_t*)handle
                title:(hb_title_t*)title;
- (NSImage *) imageForPicture: (NSInteger) pictureIndex;
- (void) purgeImageCache;
@end

