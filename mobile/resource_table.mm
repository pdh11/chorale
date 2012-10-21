#import "resource_table.h"
#import <UIKit/UIImageAndTextTableCell.h>
#import <UIKit/UISimpleTableCell.h>
#include "libdbreceiver/db.h"
#include "libdbupnp/db.h"
#include "libdblocal/db.h"
#include "context.h"

@implementation ResourceTable

- (id)initWithFrame:(CGRect)rect
{

    Srsly("RT1\n");

    self = [ super initWithFrame: rect ];
	  Srsly("RT2\n");
    if (nil != self) {
	Srsly("RT3\n");
	Resource rs;
	rs.m_name = "Hamsters on Amphetamines";
	  Srsly("RT4\n");
	m_resources.push_back(rs);
	  Srsly("RT5\n");

	m_column = [ [ UITableColumn alloc ]
		       initWithTitle: @"Column 1"
		       identifier: @"column1"
		       width: rect.size.width ];

	  Srsly("RT6\n");
	[ self addTableColumn: m_column ];
	[ self setDataSource: self ];
	[ self setSeparatorStyle: 1 ];
	[ self reloadData ]; // Obviously

	  Srsly("RT7\n");
    }

    return self;
}

- (void) dealloc
{
    [ m_column release ];
    m_resources.clear();
    [ super dealloc ];
}

- (int) numberOfRowsInTable: (UITable*)table
{
    return m_resources.size();
}

- (UITableCell*) table: (UITable*)table
	    cellForRow: (int)row
		column: (UITableColumn*)col
{
    Srsly("RT8\n");
    UIImageAndTextTableCell *cell = [ [ UIImageAndTextTableCell alloc ] init ];
    [ cell setTitle: [ NSString stringWithUTF8String: m_resources[row].m_name.c_str() ] ];
    [ cell setImage: [ UIImage applicationImageNamed: @"network.png" ] ];
    [ cell setShowDisclosure: YES ];
    [ cell setDisclosureStyle: 1 ];
	  Srsly("RT9\n");
    return [ cell autorelease ];
}

@end
