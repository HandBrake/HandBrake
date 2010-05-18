/* $Id: HBPreviewController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#include "hb.h"
/* Needed for Quicktime movie previews */
#import <QTKit/QTKit.h> 

@class HBController;

#define HB_NUM_HBLIB_PICTURES      20   // # of preview pictures libhb should generate

@interface PreviewController : NSWindowController
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    HBController             *fHBController;        // reference to HBController
    
    IBOutlet NSWindow        * fPreviewWindow;
    NSWindow                 * fFullScreenWindow; // Full Screen window
    NSMutableDictionary      * fPicturePreviews;  // NSImages, one for each preview libhb creates, created lazily
    int                        fPicture;

    IBOutlet NSImageView     * fPictureView;
    IBOutlet NSBox           * fPictureViewArea;
    IBOutlet NSBox           * fPictureControlBox;
    IBOutlet NSBox           * fEncodingControlBox;
    IBOutlet NSBox           * fMoviePlaybackControlBox;

    IBOutlet NSSlider        * fPictureSlider;
    IBOutlet NSTextField     * fInfoField;
    IBOutlet NSTextField     * fscaleInfoField;
    
    BOOL                     isEncoding;

	
    int                      MaxOutputWidth;
    int                      MaxOutputHeight;

    int output_width, output_height, output_par_width, output_par_height;
    int display_width;
    
    /* Hud Control Overlay */
    NSTimer                         * fHudTimer;
    int                               hudTimerSeconds;
    
    /* Full Screen Mode Toggle */
    IBOutlet NSButton               * fScaleToScreenToggleButton;
    IBOutlet NSButton               * fPictureSettingsToggleButton;
    BOOL                              scaleToScreen;
    
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
    NSTimer                         * fLibhbTimer;             // timer for retrieving state from libhb
    IBOutlet NSTextField            * fPreviewMovieStatusField; 
    IBOutlet NSPopUpButton          * fPreviewMovieLengthPopUp; // popup of choices for length of preview in seconds
}
- (id)init;

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void)setHBController: (HBController *)controller;
- (IBAction) showPreviewWindow: (id)sender;
- (BOOL)acceptsMouseMovedEvents;
- (void) displayPreview;

- (IBAction) SettingsChanged: (id) sender;
- (IBAction) pictureSliderChanged: (id) sender;
- (IBAction)showPictureSettings:(id)sender;
- (NSString*) pictureSizeInfoString;

- (IBAction)toggleScaleToScreen:(id)sender;
- (IBAction)goWindowedScreen:(id)sender;

/* HUD overlay */
- (void) startHudTimer;
- (void) stopHudTimer;

/* Movie Previews */
- (void) startReceivingLibhbNotifications;
- (void) stopReceivingLibhbNotifications;

- (void) installMovieCallbacks;
- (void)removeMovieCallbacks;

- (IBAction) createMoviePreview: (id) sender;
- (void) libhbStateChanged: (hb_state_t ) state;
- (IBAction) showMoviePreview: (NSString *) path;
- (IBAction) toggleMoviePreviewPlayPause: (id) sender;
- (IBAction) moviePlaybackGoToBeginning: (id) sender;
- (IBAction) moviePlaybackGoToEnd: (id) sender;
- (IBAction) moviePlaybackGoBackwardOneFrame: (id) sender;
- (IBAction) moviePlaybackGoForwardOneFrame: (id) sender;

-(void) initPreviewScrubberForMovie;
-(void) adjustPreviewScrubberForCurrentMovieTime;
- (IBAction) previewScrubberChanged: (id) sender;
-(void)setTime:(int)timeValue;
-(void)timeToQTTime:(long)timeValue resultTime:(QTTime *)aQTTime;
- (void) startMovieTimer;
- (void) stopMovieTimer;
- (NSString*) calculatePlaybackSMTPETimecodeForDisplay;


- (IBAction) previewDurationPopUpChanged: (id) sender;


- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title;

+ (NSImage *) makeImageForPicture: (int)pictureIndex
                libhb:(hb_handle_t*)handle
                title:(hb_title_t*)title;
- (NSImage *) imageForPicture: (int) pictureIndex;
- (void) purgeImageCache;
@end

