#import "main.h"
#import "resource_table.h"
#include "context.h"
#include <boost/thread/mutex.hpp>
#include "libutil/trace.h"

int main(int argc, char **argv)
{
    NSAutoreleasePool *autoreleasePool = [ [ NSAutoreleasePool alloc ] init ];
    int returnCode = UIApplicationMain(argc, argv, 
				       @"ChoraleApp", @"ChoraleApp");
    [ autoreleasePool release ];
    return returnCode;
}

@implementation ChoraleApp

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    window = [ [ UIWindow alloc ] initWithContentRect:
        [ UIHardware fullScreenApplicationContentRect ]
    ];

    Srsly( "DFL1\n");

    m_context = new Context;
	   Srsly( "DFL2\n");

    m_context->Init();
	   Srsly( "DFL3\n");


    CGRect rect = [ UIHardware fullScreenApplicationContentRect ];
    rect.origin.x = rect.origin.y = 0.0f;

    mainView = [ [ MainView alloc ] initWithFrame: rect ];

	   Srsly( "DFL4\n");

    [ window setContentView: mainView ];
	   Srsly( "DFL5\n");
    [ window orderFront: self ];
	   Srsly( "DFL6\n");
    [ window makeKey: self ];
	   Srsly( "DFL7\n");
//    [ window _setHidden: NO ];

	   Srsly( "DFL8\n");
}

- (void)applicationWillTerminate:(UIApplication*)application
{
    delete m_context;
    m_context = NULL;
}

@end

@implementation MainView
- (id)initWithFrame:(CGRect)rect {
	Srsly("MV1\n");

    self = [ super initWithFrame: rect ];
	Srsly("MV2\n");

    if (nil != self) {
	Srsly("MV3\n");
        m_resource_table = [ [ ResourceTable alloc ] initWithFrame: rect ];
	Srsly("MV4\n");
        [ self addSubview: m_resource_table ];
	Srsly("MV5\n");
    }
	Srsly("MV6\n");

    return self;
}

- (void)dealloc
{
    [ super dealloc ];
}

@end
