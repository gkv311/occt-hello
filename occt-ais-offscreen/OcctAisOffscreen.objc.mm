#import <Cocoa/Cocoa.h>

#include <Cocoa_LocalPool.hxx>
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
  - (void ) applicationDidFinishLaunching: (NSNotification* )theNotification {}
  - (void ) applicationWillTerminate: (NSNotification* )theNotification {}

  + (void ) doDummyThread: (id )theParam {}

@end

void occtNSAppCreate()
{
  // create dummy NSThread to ensure Cocoa thread-safety
  [NSThread detachNewThreadSelector: @selector(doDummyThread: ) toTarget: [OcctTestNSResponder class] withObject: nullptr];

  NSApplication* anAppNs = [NSApplication sharedApplication];
  OcctTestNSResponder* anAppResp = [OcctTestNSResponder sharedInstance];
  [anAppNs setDelegate: anAppResp];
  //[anAppNs run]; // Cocoa event loop
}
