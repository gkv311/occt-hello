#import <Cocoa/Cocoa.h>

#include <AIS_ViewController.hxx>
#include <Cocoa_LocalPool.hxx>
#include <Cocoa_Window.hxx>
#include <Message.hxx>

//! Main Cocoa application responder - minimal implementation for offscreen viewer.
@interface OcctTestNSResponder : NSObject <NSApplicationDelegate>
  {
  }

  //! Default constructor.
  - (id ) init;

  //! Access singleton.
  + (OcctTestNSResponder* ) sharedInstance;

  //! Dummy method for thread-safety Cocoa initialization.
  + (void ) doDummyThread: (id )theParam;

@end

//! Custom Cocoa view to handle events
@interface OcctAisCocoaEventManagerView : NSView
@end

//! Custom Cocoa window delegate to handle window events
@interface OcctAisCocoaWindowController : NSObject <NSWindowDelegate>
@end

@implementation OcctTestNSResponder

  - (id )init
  {
    self = [super init];
    return self;
  }

  // Singletone implementation
  + (OcctTestNSResponder* ) sharedInstance
  {
    static OcctTestNSResponder* TheAppResponder = [[super allocWithZone: nullptr] init];
    return TheAppResponder;
  }
  + (id ) allocWithZone: (NSZone* )theZone { return [[self sharedInstance] retain]; }
  - (id ) copyWithZone: (NSZone* )theZone { return self; }
  - (id ) retain { return self; }
  - (NSUInteger ) retainCount { return NSUIntegerMax; }
  - (oneway void ) release {}
  - (id ) autorelease { return self; }

  - (BOOL ) application: (NSApplication* )theApplication openFile: (NSString* )theFilename { return YES; }
  - (void ) applicationWillFinishLaunching: (NSNotification* )theNotif { [self createMenu]; }
  - (void ) applicationDidFinishLaunching: (NSNotification* )theNotif {}
  - (void ) applicationWillTerminate: (NSNotification* )theNotif {}

  + (void ) doDummyThread: (id )theParam {}

  - (void ) createMenu
  {
    NSMenu* aMenubar = [[NSMenu alloc] init];
    NSMenuItem* aMenuBarItem = [[NSMenuItem alloc] init];
    [aMenubar addItem: aMenuBarItem];
    [NSApp setMainMenu: aMenubar];
    NSMenu* aMenu = [[NSMenu alloc] init];
    NSString* aQuitTitle = @"Quit";
    NSMenuItem* aQuitMenuItem = [[NSMenuItem alloc] initWithTitle: aQuitTitle action: @selector(terminate:) keyEquivalent: @"q"];
    [aMenu addItem: aQuitMenuItem];
    [aMenuBarItem setSubmenu: aMenu];
  }

@end

@implementation OcctAisCocoaWindowController

- (void )windowWillClose: (NSNotification* )theNotif
{
  (void )theNotif;
  [[NSApplication sharedApplication] terminate: nil];
}

- (void )windowDidBecomeKey: (NSNotification* )theNotif
{
  //NSWindow* aWindow = [theNotif object];
  //ActivateView(aWindow);
}

@end

//! Retrieve cursor position
static Graphic3d_Vec2i getMouseCoords(NSView* theView, NSEvent* theEvent)
{
  NSPoint aMouseLoc = [theView convertPoint: [theEvent locationInWindow] fromView: nil];
  NSRect  aBounds   = [theView bounds];
  return Graphic3d_Vec2i(int(aMouseLoc.x), int(aBounds.size.height - aMouseLoc.y));
}

//! Convert key flags from mouse event.
static Aspect_VKeyFlags getMouseKeyFlags (NSEvent* theEvent)
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if (([theEvent modifierFlags] & NSEventModifierFlagShift) != 0)
    aFlags |= Aspect_VKeyFlags_SHIFT;

  if (([theEvent modifierFlags] & NSEventModifierFlagControl) != 0)
    aFlags |= Aspect_VKeyFlags_CTRL;

  if (([theEvent modifierFlags] & NSEventModifierFlagOption) != 0)
    aFlags |= Aspect_VKeyFlags_ALT;

  return aFlags;
}

@implementation OcctAisCocoaEventManagerView

AIS_ViewController* myViewCtrl = nullptr;

- (void )setViewController: (AIS_ViewController* )theCtrl
{
  myViewCtrl = theCtrl;
}

- (void )updateTrackingAreas
{
  // to track mouse movements for dynamic highlight
  NSTrackingAreaOptions aFlags = (NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
  NSTrackingArea* anArea = [[NSTrackingArea alloc] initWithRect: [self bounds] options: aFlags owner: self userInfo: nil];
  [self addTrackingArea: anArea];
}

- (void )setFrameSize: (NSSize )theNewSize
{
  [super setFrameSize: theNewSize];
  myViewCtrl->ProcessConfigure(true);
}

- (void )drawRect: (NSRect )theDirtyRect
{
  (void )theDirtyRect;
  if (myViewCtrl != nullptr)
    myViewCtrl->ProcessExpose();
}

- (void )mouseMoved: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);
  const Aspect_VKeyMouse aButtons = myViewCtrl->PressedMouseButtons();
  myViewCtrl->UpdateMousePosition(aPos, aButtons, aFlags, false);
  myViewCtrl->ProcessInput();
}

- (BOOL )acceptsFirstResponder
{
  return YES;
}

- (void )mouseDown: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);
  myViewCtrl->PressMouseButton(aPos, Aspect_VKeyMouse_LeftButton, aFlags, false);
  myViewCtrl->ProcessInput();
}

- (void )mouseUp: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);
  myViewCtrl->ReleaseMouseButton(aPos, Aspect_VKeyMouse_LeftButton, aFlags, false);
  myViewCtrl->ProcessInput();
}

- (void )mouseDragged: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);
  const Aspect_VKeyMouse aButtons = myViewCtrl->PressedMouseButtons();
  myViewCtrl->UpdateMousePosition(aPos, aButtons, aFlags, false);
  myViewCtrl->ProcessInput();
}

- (void )rightMouseDown: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);
  myViewCtrl->PressMouseButton(aPos, Aspect_VKeyMouse_RightButton, aFlags, false);
  myViewCtrl->ProcessInput();
}

- (void )rightMouseUp: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);
  myViewCtrl->ReleaseMouseButton(aPos, Aspect_VKeyMouse_RightButton, aFlags, false);
  myViewCtrl->ProcessInput();
}

- (void )rightMouseDragged: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);
  const Aspect_VKeyMouse aButtons = myViewCtrl->PressedMouseButtons();
  myViewCtrl->UpdateMousePosition (aPos, aButtons, aFlags, false);
  myViewCtrl->ProcessInput();
}

- (void )scrollWheel: (NSEvent* )theEvent
{
  const Graphic3d_Vec2i  aPos   = getMouseCoords(self, theEvent);
  const Aspect_VKeyFlags aFlags = getMouseKeyFlags(theEvent);

  const double aDelta = [theEvent deltaY];
  if (Abs (aDelta) < 0.001) // a lot of values near zero can be generated by touchpad
    return;

  myViewCtrl->UpdateMouseScroll(Aspect_ScrollDelta(aPos, aDelta, aFlags));
  myViewCtrl->ProcessInput();
}

- (void )keyDown: (NSEvent* )theEvent
{
  unsigned int aKeyCode = [theEvent keyCode];
  const Aspect_VKey aVKey = Cocoa_Window::VirtualKeyFromNative(aKeyCode);
  if (aVKey != Aspect_VKey_UNKNOWN)
  {
    const double aTimeStamp = [theEvent timestamp];
    myViewCtrl->KeyDown(aVKey, aTimeStamp);
    myViewCtrl->ProcessInput();
  }
}

- (void )keyUp: (NSEvent* )theEvent
{
  unsigned int aKeyCode = [theEvent keyCode];
  const Aspect_VKey aVKey = Cocoa_Window::VirtualKeyFromNative(aKeyCode);
  if (aVKey != Aspect_VKey_UNKNOWN)
  {
    const double aTimeStamp = [theEvent timestamp];
    myViewCtrl->KeyUp(aVKey, aTimeStamp);
    myViewCtrl->ProcessInput();
  }
}

@end

void occtNSAppCreate()
{
  // create dummy NSThread to ensure Cocoa thread-safety
  [NSThread detachNewThreadSelector: @selector(doDummyThread: ) toTarget: [OcctTestNSResponder class] withObject: nullptr];

  NSApplication* anAppNs = [NSApplication sharedApplication];
  OcctTestNSResponder* anAppResp = [OcctTestNSResponder sharedInstance];
  [anAppNs setDelegate: anAppResp];
  //[NSBundle loadNibNamed: @"MainMenu" owner: anAppResp];

  // allow our application to steal input focus (when needed)
  [anAppNs activateIgnoringOtherApps: YES];
}

void occtNSAppRun()
{
  NSApplication* anAppNs = [NSApplication sharedApplication];
  //[[NSApp mainWindow] makeKeyAndOrderFront: anAppNs];
  [anAppNs run]; // Cocoa event loop
}

void occtNSAppSetEventView(const Handle(Cocoa_Window)& theWindow, AIS_ViewController* theCtrl)
{
  if (theWindow.IsNull())
    return;

  NSWindow* aWin = [theWindow->HView() window];
  NSRect aBounds = [[aWin contentView] bounds];

  OcctAisCocoaEventManagerView* aView = [[OcctAisCocoaEventManagerView alloc] initWithFrame: aBounds];
  [aView setViewController: theCtrl];

  // replace content view in the window
  theWindow->SetHView(aView);

  // set delegate for window
  OcctAisCocoaWindowController* aWindowController = [[[OcctAisCocoaWindowController alloc] init] autorelease];
  [aWin setDelegate: aWindowController];

  // make view as first responder in winow to capture all useful events
  [aWin makeFirstResponder: aView];
  [aWin setAcceptsMouseMovedEvents: YES];

  // should be retained by parent NSWindow
  [aView release];
}
