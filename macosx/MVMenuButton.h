#import <Cocoa/Cocoa.h>

@interface MVMenuButton : NSButton <NSCoding> {
@protected
	BOOL _drawsArrow;
	NSImage *_orgImage;
	NSImage *_smallImage;
	NSControlSize _size;
	NSToolbarItem *_toolbarItem;
}
- (NSControlSize) controlSize;
- (void) setControlSize:(NSControlSize) controlSize;

- (NSImage *) smallImage;
- (void) setSmallImage:(NSImage *) image;

- (NSToolbarItem *) toolbarItem;
- (void) setToolbarItem:(NSToolbarItem *) item;

- (BOOL) drawsArrow;
- (void) setDrawsArrow:(BOOL) arrow;
@end
