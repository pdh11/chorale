#include "config.h"

#if HAVE_COREFOUNDATION_CFRUNLOOP_H

#import <CoreFoundation/CoreFoundation.h>
#import <UIKit/UIKit.h>
#import <UIKit/UITextView.h>
#import "resource_table.h"
#include "libuidarwin/scheduler.h"

#ifdef __OBJC__

@interface MainView : UIView
{
    ResourceTable *m_resource_table;
}

- (id)initWithFrame:(CGRect)frame;
- (void)dealloc;

@end

class Context;

@interface ChoraleApp : UIApplication
{
    UIWindow *window;
    MainView *mainView;

    Context *m_context;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (void)applicationWillTerminate:(UIApplication*)application;

@end

#endif __OBJC__

#endif // HAVE_COREFOUNDATION_CFRUNLOOP_H
